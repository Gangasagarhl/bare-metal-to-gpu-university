// mp1_core.h - MP1 starter: the arch-neutral part of the regression kernel.
// One source file, compiled unchanged for x86-64 (mp1_x86.cc) and AArch64 (mp1_a64.cc).
// Each port fills in a Platform record; the core never asks "which CPU is this?".
//
// Serial protocol (the harness mp1ci.py parses exactly these line shapes):
//   MP1 BEGIN arch=<name> boot=<how the kernel was started>
//   MP1 STAGE <name> ticks=<raw counter> [us=<microseconds since entry>]
//   MP1 TEST <name> PASS|FAIL <detail>
//   MP1 SKIP <name> <reason>
//   MP1 END pass=<n> fail=<n> skip=<n>
#pragma once
#include <cstddef>
#include <cstdint>

namespace mp1 {

// How many CPUs this kernel can track. A port starts at most this many; the "cpus" test
// fails if the firmware lists more. Build with -DMP1_MAX_CPUS=<n> to change it.
#ifndef MP1_MAX_CPUS
#define MP1_MAX_CPUS 16
#endif
inline constexpr int kMaxCpus = MP1_MAX_CPUS;

struct Platform {
    const char* arch;              // printed only
    const char* boot;              // "multiboot1" or "devicetree", printed only
    const void* dtb;               // devicetree blob or nullptr (passed to the F4-23 tests)
    uint64_t page_size;            // from the arch layer
    const char* bootargs;          // kernel command line, may be empty, never nullptr
    uint64_t entry_ticks;          // counter value read first thing in kmain
    uint64_t tick_hz;              // counter frequency, 0 if the port does not know it
    uint64_t (*now)();             // reads the same counter
    int (*count_cpus)();           // CPUs the firmware tables list, -1 if unknown
    // Looks for a root block device, waiting up to wait_ms (rescanning) if none is there.
    // Writes a one-line description into desc. nullptr: this port has no storage driver yet.
    bool (*find_root_device)(uint64_t wait_ms, char* desc, size_t n);
};

// Runs every stage and test, prints the protocol lines, returns the number of failures.
int run(const Platform& p);

} // namespace mp1
