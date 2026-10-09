// ext2tool.cc - F3-33: command-line front end of the ext2 driver, for the lab script.
//   ext2tool info|ls|cat|put|mkdir|rm|stress|crash <image> [arguments]
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <string>
#include "ext2.h"

static int stress(Ext2& fs, int ops, unsigned seed)
{
    std::mt19937 rng(seed);
    if (fs.mkdir("/logs") != 0) { std::printf("mkdir /logs failed\n"); return 1; }
    std::vector<std::string> pool;
    for (int i = 0; i < 30; ++i)
        pool.push_back((i % 3 == 0 ? "/logs/" : "/") + std::string("stress-") + std::to_string(i) + ".dat");
    std::map<std::string, std::vector<std::uint8_t>> model;
    int creates = 0, replaces = 0, deletes = 0, reads = 0, big = 0;
    for (int i = 0; i < ops; ++i) {
        const std::string& path = pool[rng() % pool.size()];
        int what = static_cast<int>(rng() % 10);
        if (what < 5) {
            std::size_t size = rng() % 20 == 0 ? 270000 + rng() % 30000 : rng() % 20000;  // some need double indirect
            std::vector<std::uint8_t> data(size);
            for (auto& b : data) b = static_cast<std::uint8_t>(rng());
            if (size >= 270000) ++big;
            if (fs.write_file(path, data) != 0) { std::printf("op %d: write %s failed\n", i, path.c_str()); return 1; }
            (model.count(path) ? replaces : creates)++;
            model[path] = std::move(data);
        } else if (what < 8) {
            if (!model.count(path)) continue;
            if (fs.unlink(path) != 0) { std::printf("op %d: unlink %s failed\n", i, path.c_str()); return 1; }
            model.erase(path);
            ++deletes;
        } else {
            auto got = fs.read_file(path);
            bool want = model.count(path) != 0;
            if (got.has_value() != want || (want && *got != model[path])) {
                std::printf("op %d: read %s does not match the model\n", i, path.c_str());
                return 1;
            }
            ++reads;
        }
    }
    for (auto& [path, data] : model) {
        auto got = fs.read_file(path);
        if (!got || *got != data) { std::printf("final check: %s differs\n", path.c_str()); return 1; }
    }
    std::printf("%d operations: %d creates, %d replaces (%d files over 268 KiB), %d unlinks, %d reads checked\n",
                ops, creates, replaces, big, deletes, reads);
    std::printf("files at the end: %zu, all equal to the model; free blocks %u, free inodes %u\n",
                model.size(), fs.free_blocks(), fs.free_inodes());
    return 0;
}

// The test run of the forensic lab: one file written while the power fails after `cut`
// block writes, then a reboot without fsck and a second file.
static int crash(const char* img, long cut)
{
    std::vector<std::uint8_t> a(20 * 1024, 'A'), b(20 * 1024, 'B');
    {
        Disk d(img, 1024);
        d.cut_after = cut;
        Ext2 fs(d);
        if (!fs.mount()) return 1;
        std::printf("== run 1: write /report-a.txt (20480 bytes of 'A'); power fails after write %ld ==\n", cut);
        fs.write_file("/report-a.txt", a);
        for (std::size_t i = 0; i < d.log.size(); ++i)
            std::printf("write %2zu: block %5u  %-34s %s\n", i + 1, d.log[i].block, fs.role(d.log[i].block).c_str(),
                        d.log[i].reached ? "" : "LOST (no power)");
    }
    Disk d(img, 1024);
    Ext2 fs(d);
    if (!fs.mount()) return 1;
    std::printf("== run 2 (after reboot, no fsck): write /report-b.txt (20480 bytes of 'B') ==\n");
    int rc = fs.write_file("/report-b.txt", b);
    std::printf("write /report-b.txt: %s\n", rc == 0 ? "ok" : "failed");
    auto got = fs.read_file("/report-a.txt");
    if (!got) { std::printf("/report-a.txt: not found\n"); return 0; }
    std::size_t still = 0;
    for (std::uint8_t c : *got) still += c == 'A';
    std::printf("/report-a.txt: %zu bytes, %zu of them still 'A'\n", got->size(), still);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3) { std::fprintf(stderr, "usage: ext2tool info|ls|cat|put|mkdir|rm|stress|crash <image> ...\n"); return 2; }
    std::string cmd = argv[1];
    if (cmd == "crash" && argc > 3) return crash(argv[2], std::atol(argv[3]));
    Disk dev(argv[2], 1024);
    Ext2 fs(dev);
    if (!dev.ok() || !fs.mount()) { std::printf("mount failed: not an ext2 volume this driver accepts\n"); return 1; }
    std::string path = argc > 3 ? argv[3] : "/";
    int rc = 0;
    if (cmd == "info") {
        fs.print_info();
    } else if (cmd == "ls") {
        auto v = fs.list(path);
        if (!v) { std::printf("ls %s: not found\n", path.c_str()); return 1; }
        std::printf("directory %s:\n", path.c_str());
        for (const DirEntry& e : *v)
            std::printf("  inode %4u  type %u  %s\n", e.inode, e.type, e.name.c_str());
    } else if (cmd == "cat") {
        auto d = fs.read_file(path);
        if (!d) { std::printf("cat %s: not found\n", path.c_str()); return 1; }
        std::fwrite(d->data(), 1, d->size(), stdout);
    } else if (cmd == "put" && argc > 4) {
        std::ifstream in(argv[4], std::ios::binary);
        std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        rc = fs.write_file(path, data);
        std::printf("put %s (%zu bytes): %s\n", path.c_str(), data.size(), rc == 0 ? "ok" : "failed");
    } else if (cmd == "mkdir") {
        rc = fs.mkdir(path);
        std::printf("mkdir %s: %s\n", path.c_str(), rc == 0 ? "ok" : "failed");
    } else if (cmd == "rm") {
        rc = fs.unlink(path);
        std::printf("rm %s: %s\n", path.c_str(), rc == 0 ? "ok" : "failed");
    } else if (cmd == "stress" && argc > 4) {
        rc = stress(fs, std::atoi(argv[3]), static_cast<unsigned>(std::atoi(argv[4])));
    } else {
        std::fprintf(stderr, "unknown command\n");
        return 2;
    }
    return rc == 0 ? 0 : 1;
}
