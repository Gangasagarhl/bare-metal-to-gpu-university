// neutral_tests.h - F4-23: the arch-neutral kernel test suite of DR402.
// The same source runs on x86-64 (F4-30), AArch64 (F4-24 ...) and RISC-V (F4-28 ...).
// Each port passes in only what differs between platforms; the tests never ask "which CPU?".
#pragma once
#include <cstdint>

struct NeutralEnv {
    const char* arch_name;     // printed only, never tested
    const void* dtb;           // devicetree blob, or nullptr on platforms that use ACPI/Multiboot
    uint64_t page_size;        // the arch layer's page size: core code must not assume 4096
};

// Prints one line per test and a summary line; returns the number of failures.
int run_neutral_tests(const NeutralEnv& env);
