// test_replay.cc - trace replay: the access trace QEMU recorded while the F4-17 kernel ran
// (golden_trace.out) is played back to the same driver code on the host. Every read returns
// the recorded value; every write must match the recorded offset and value. The program does
// exactly what f417_main.cc does: probe, compute n = 0..14, dump.
#include "edu4.h"
#include "shim_host.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Rec { bool write; uint32_t off, value; };

struct ReplayBackend : Backend {
    std::vector<Rec> recs;
    std::size_t next = 0;
    int mismatches = 0;
    void complain(const char* what, uint32_t off, uint32_t v)
    {
        if (mismatches++ < 5)
            std::printf("MISMATCH at access %zu: driver %s 0x%02x (0x%08x); recorded %s\n", next + 1, what, off, v,
                        next < recs.size() ? (recs[next].write ? "a write" : "a read") : "nothing (trace ended)");
    }
    uint32_t read(uint32_t off) override
    {
        if (next >= recs.size() || recs[next].write || recs[next].off != off) { complain("reads", off, 0); return 0; }
        return recs[next++].value;
    }
    void write(uint32_t off, uint32_t v) override
    {
        if (next >= recs.size() || !recs[next].write || recs[next].off != off || recs[next].value != v) {
            complain("writes", off, v);
            return;
        }
        ++next;
    }
};

int main(int argc, char** argv)
{
    ReplayBackend b;
    std::ifstream f(argc > 1 ? argv[1] : "golden_trace.out");
    std::string line;
    while (std::getline(f, line)) {
        int seq = 0;
        char rw = 0;
        unsigned off = 0, val = 0;
        if (std::sscanf(line.c_str(), "#%d %c 0x%x 0x%x", &seq, &rw, &off, &val) == 4)
            b.recs.push_back({rw == 'W', off, val});
    }
    std::cout << "recorded accesses: " << b.recs.size() << '\n';
    g_backend = &b;
    pci4::Function fn{};
    fn.vendor = 0x1234;
    fn.device = 0x11E8;
    edu4::Device d;
    std::cout << "probe: " << edu4::to_string(edu4::probe(fn, d)) << '\n';
    for (uint32_t n = 0; n <= 14; ++n) {
        uint32_t r = 0;
        const edu4::Err e = edu4::compute(d, n, r);
        if (e != edu4::Err::Ok) std::cout << "compute(" << n << "): " << edu4::to_string(e) << '\n';
    }
    edu4::dump(d);
    std::cout << "replayed " << b.next << " of " << b.recs.size() << " accesses, " << b.mismatches << " mismatch(es): "
              << (b.mismatches == 0 && b.next == b.recs.size() ? "PASS" : "FAIL") << '\n';
    return b.mismatches == 0 && b.next == b.recs.size() ? 0 : 1;
}
