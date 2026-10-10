// read_stack.cpp - where does read() go inside the running Linux kernel?
// A reader thread calls read() in a loop on a file opened with O_DIRECT (so every call goes to
// the disk, not the page cache). The main thread samples that thread's kernel stack from
// /proc/self/task/<tid>/stack, prints the most frequent chain and the first chain that reaches
// the block driver, and checks which function names this kernel has.
// Linux only. Reading a task's kernel stack needs root privileges (this build ran as root).
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
constexpr std::size_t kBlock = 4096;
constexpr int kMiB = 32;
constexpr int kSamples = 400;

std::vector<std::string> frames(const std::string& text)  // "[<0>] vfs_read+0x237/0x360"
{
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        std::size_t b = line.find("] ");
        if (b == std::string::npos) continue;
        out.push_back(line.substr(b + 2, line.find('+') - (b + 2)));
    }
    return out;
}

int main()
{
    struct utsname uts {};
    uname(&uts);
    std::cout << "kernel: " << uts.sysname << " " << uts.release << " " << uts.machine << "\n";
    const std::string path = "read_stack.tmp";
    {
        std::ofstream out(path, std::ios::binary);
        const std::vector<char> mib(1024 * 1024, 'x');
        for (int i = 0; i < kMiB; ++i) out.write(mib.data(), std::streamsize(mib.size()));
    }
    int wfd = open(path.c_str(), O_RDONLY);
    fsync(wfd);  // no write-back left pending: the samples should show reads only
    close(wfd);

    struct stat st {};
    stat(path.c_str(), &st);
    std::string devno = std::to_string(major(st.st_dev)) + ":" + std::to_string(minor(st.st_dev));
    std::error_code ec;
    fs::path sys = fs::canonical("/sys/dev/block/" + devno, ec);
    fs::path drv = fs::read_symlink(sys / "device" / "driver", ec);
    if (ec) drv = fs::read_symlink(sys.parent_path() / "device" / "driver", ec);  // a partition
    std::cout << "file " << path << " (" << kMiB << " MiB) is on block device " << devno << " ("
              << sys.filename().string() << "), driver " << drv.filename().string() << "\n";

    std::atomic<pid_t> tid{0};
    std::atomic<bool> stop{false};
    std::atomic<long> reads{0};
    std::thread reader([&]() {
        tid = gettid();
        int fd = open(path.c_str(), O_RDONLY | O_DIRECT);
        std::unique_ptr<void, decltype(&std::free)> buf(std::aligned_alloc(kBlock, kBlock),
                                                         &std::free);
        while (fd >= 0 && !stop) {
            if (read(fd, buf.get(), kBlock) <= 0) lseek(fd, 0, SEEK_SET);  // at the end: rewind
            ++reads;
        }
        if (fd >= 0) close(fd);
    });
    while (tid == 0) std::this_thread::yield();

    // Sample at least kSamples stacks; go on (up to 6 s) until one chain reaches the driver.
    std::map<std::vector<std::string>, int> chains;
    std::vector<std::string> driverChain;
    int samples = 0;
    int unreadable = 0;
    const std::string stackFile = "/proc/self/task/" + std::to_string(tid) + "/stack";
    const auto t0 = std::chrono::steady_clock::now();
    const auto limit = std::chrono::seconds(6);
    while (samples < kSamples ||
           (driverChain.empty() && std::chrono::steady_clock::now() - t0 < limit)) {
        std::ifstream in(stackFile);
        std::stringstream text;
        text << in.rdbuf();
        std::vector<std::string> f = frames(text.str());
        ++samples;
        if (f.empty()) ++unreadable; else ++chains[f];
        for (const auto& name : f) {
            if (driverChain.empty() && name.ends_with("_queue_rq")) driverChain = f;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    stop = true;
    reader.join();
    fs::remove(path);

    std::cout << "reader thread: read() of " << kBlock << " bytes with O_DIRECT, called "
              << (reads > 1000 ? "more than 1000" : "fewer than 1000") << " times\n";
    std::cout << "kernel stacks sampled: " << (samples < 1000 ? "fewer than 1000" : "1000 or more")
              << "; distinct call chains: " << (chains.size() < 10 ? "fewer than 10" : "10 or more")
              << "\n";
    int top = 0;
    std::vector<std::string> common;
    for (const auto& [f, n] : chains) {
        if (n > top) { top = n; common = f; }
    }
    std::cout << "\nmost frequent chain, " << (2 * top > samples ? "more" : "less")
              << " than half of all samples (innermost first):\n";
    for (const auto& name : common) std::cout << "    " << name << "\n";
    if (driverChain.empty()) {
        std::cout << "\nno sample reached a *_queue_rq function this time (sampling is luck)\n";
    } else {
        std::cout << "\nfirst chain that reached the block driver (innermost first):\n";
        for (const auto& name : driverChain) std::cout << "    " << name << "\n";
    }

    std::set<std::string> known;
    std::ifstream syms("/proc/kallsyms");
    std::string addr, type, name;
    while (syms >> addr >> type >> name) {
        known.insert(name);
        std::getline(syms, addr);  // skip an optional [module] column
    }
    std::cout << "\nin this kernel's symbol table (/proc/kallsyms):\n";
    for (const char* s : {"ksys_read", "vfs_read", "ext4_file_read_iter", "iomap_dio_rw",
                          "submit_bio", "blk_mq_submit_bio", "virtio_queue_rq", "nvme_queue_rq"}) {
        std::cout << "    " << s << (known.count(s) ? "  present" : "  absent") << "\n";
    }
    return unreadable == samples ? 1 : 0;
}
