// fat32tool.cc - F3-32: command-line front end of the FAT32 driver, for the lab script.
//   fat32tool info|ls|cat|put|rm|stress <image> [arguments]
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <string>
#include "fat32.h"

static void show(const std::vector<Entry>& v)
{
    for (const Entry& e : v) {
        if (e.is_dir()) std::printf("  %-10s", "<DIR>"); else std::printf("  %10u", e.size);
        std::printf("  %-12s  %s\n", e.short_name.c_str(), e.long_name.c_str());
    }
}

// 10,000 random create / replace / delete / read operations checked against an in-memory model.
static int stress(Fat32& fs, int ops, unsigned seed)
{
    std::mt19937 rng(seed);
    std::vector<std::string> pool;
    for (int i = 0; i < 40; ++i) {
        char n[64];
        if (i % 4 == 0) std::snprintf(n, sizeof n, "FILE%04d.BIN", i);                // plain 8.3
        else std::snprintf(n, sizeof n, "Stress test file number %02d with a long name.txt", i);
        pool.push_back(n);
    }
    std::map<std::string, std::vector<std::uint8_t>> model;
    int creates = 0, replaces = 0, deletes = 0, reads = 0;
    for (int i = 0; i < ops; ++i) {
        const std::string& name = pool[rng() % pool.size()];
        std::string path = "/" + name;
        int what = static_cast<int>(rng() % 10);
        if (what < 5) {                                               // create or replace
            std::vector<std::uint8_t> data(rng() % 6000);
            for (auto& b : data) b = static_cast<std::uint8_t>(rng());
            if (fs.write_file(path, data) != 0) { std::printf("op %d: write %s failed\n", i, name.c_str()); return 1; }
            (model.count(name) ? replaces : creates)++;
            model[name] = std::move(data);
        } else if (what < 8) {                                        // delete
            if (!model.count(name)) continue;
            if (fs.remove(path) != 0) { std::printf("op %d: remove %s failed\n", i, name.c_str()); return 1; }
            model.erase(name);
            ++deletes;
        } else {                                                      // read and compare
            auto got = fs.read_file(path);
            bool want = model.count(name) != 0;
            if (got.has_value() != want || (want && *got != model[name])) {
                std::printf("op %d: read %s does not match the model\n", i, name.c_str());
                return 1;
            }
            ++reads;
        }
    }
    for (auto& [name, data] : model) {                                // final full comparison
        auto got = fs.read_file("/" + name);
        if (!got || *got != data) { std::printf("final check: %s differs\n", name.c_str()); return 1; }
    }
    std::printf("%d operations: %d creates, %d replaces, %d deletes, %d reads checked\n",
                ops, creates, replaces, deletes, reads);
    std::printf("files at the end: %zu, all equal to the model; free clusters %u\n", model.size(), fs.free_clusters());
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3) { std::fprintf(stderr, "usage: fat32tool info|ls|cat|put|rm|stress <image> ...\n"); return 2; }
    BlockDev dev(argv[2]);
    Fat32 fs(dev);
    if (!dev.ok() || !fs.mount()) { std::printf("mount failed: not a FAT32 volume this driver accepts\n"); return 1; }
    std::string cmd = argv[1];
    std::string path = argc > 3 ? argv[3] : "/";
    int rc = 0;
    if (cmd == "info") {
        fs.print_info();
    } else if (cmd == "ls") {
        auto v = fs.list(path);
        if (!v) { std::printf("ls %s: not found\n", path.c_str()); return 1; }
        std::printf("directory %s:\n", path.c_str());
        show(*v);
    } else if (cmd == "cat") {
        auto d = fs.read_file(path);
        if (!d) { std::printf("cat %s: not found\n", path.c_str()); return 1; }
        std::fwrite(d->data(), 1, d->size(), stdout);
    } else if (cmd == "put" && argc > 4) {
        std::ifstream in(argv[4], std::ios::binary);
        std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        rc = fs.write_file(path, data);
        std::printf("put %s (%zu bytes): %s\n", path.c_str(), data.size(), rc == 0 ? "ok" : "failed");
    } else if (cmd == "rm") {
        rc = fs.remove(path);
        std::printf("rm %s: %s\n", path.c_str(), rc == 0 ? "ok" : "failed");
    } else if (cmd == "stress" && argc > 4) {
        rc = stress(fs, std::atoi(argv[3]), static_cast<unsigned>(std::atoi(argv[4])));
    } else {
        std::fprintf(stderr, "unknown command\n");
        return 2;
    }
    std::fprintf(stderr, "[device: %zu sector reads, %zu sector writes]\n", dev.reads, dev.writes);
    return rc == 0 ? 0 : 1;
}
