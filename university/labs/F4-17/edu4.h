// edu4.h - a polled driver for PCI device 1234:11e8, designed in F4-17 from the observed
// specification of F4-16. Interface: probe a PCI function, then compute factorials.
#pragma once
#include <stdint.h>
#include "pci4.h"

namespace edu4 {

enum class Err { Ok, NoDevice, Gone, BadId, CheckFailed, Timeout, StuckIrq };
const char* to_string(Err e);

struct Stats { uint32_t computes, polls, timeouts, gone; };

struct Device {
    pci4::Addr at;
    uintptr_t base;
    uint64_t timeout_ticks;     // time-stamp counter ticks allowed for one computation
    Stats stats;
};

bool matches(const pci4::Function& f);                  // bus binding: by vendor and device ID
Err probe(const pci4::Function& f, Device& d);          // init sequence + self-test
Err compute(Device& d, uint32_t n, uint32_t& result);   // the device's function
void dump(const Device& d);                             // observability: registers + counters

} // namespace edu4
