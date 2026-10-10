// fs1tool.cc - F3-44: drives the ext2 writer of ext2w.h (milestone FS1).
//   fs1tool workload <img> <stream|-> <seed> <ops> careful|careless <expectdir>
//       random operations on <img>; records the write stream; writes the expected contents
//       of every file (and of every fsynced "promised" file) into <expectdir>
//   fs1tool crash <base.img> <stream> <seed> <out.img>
//       a power cut at a random point of the stream (see stream.h); prints the promises
//       (fsyncs that had returned before the cut) that the harness must check
//   fs1tool show <stream> <first> <count>
//       prints records of a write stream: block writes, FLUSHes and fsync promises
#include "stream.h"
#include "workload.h"
#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    try {
        const std::vector<std::string> a(argv + 1, argv + argc);
        if (a.size() == 7 && a[0] == "workload") {
            const bool careful = a[5] == "careful";
            os401::BlockDev dev(a[1], 1024, a[2] == "-" ? "" : a[2]);
            os401::DirectStore store(dev, careful);
            os401::Ext2 fs(store, 1024, !careful);
            const int rc = os401::runWorkload(fs, dev, static_cast<unsigned>(std::stoul(a[3])), std::stoi(a[4]),
                                              a[2] != "-", a[6], careful ? "careful ordering" : "careless ordering",
                                              [] {});
            dev.save(a[1]);
            return rc;
        }
        if (a.size() == 4 && a[0] == "show")
            return os401::showStream(a[1], std::stoul(a[2]), std::stoul(a[3]));
        if (a.size() == 5 && a[0] == "crash")
            return os401::crashImage(a[1], a[2], static_cast<unsigned>(std::stoul(a[3])), a[4]);
        std::fprintf(stderr, "usage: see the comment at the top of fs1tool.cc\n");
        return 2;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fs1tool: %s\n", e.what());
        return 1;
    }
}
