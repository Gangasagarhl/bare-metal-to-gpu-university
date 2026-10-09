// shim_host.h - the backend that the host shims (k4.h, pci4.h) send device accesses to.
#pragma once
#include <cstdint>

struct Backend {
    virtual uint32_t read(uint32_t off) = 0;
    virtual void write(uint32_t off, uint32_t v) = 0;
    virtual ~Backend() = default;
};

constexpr uint32_t kFakeBar0 = 0xFEA00000u;   // where the fake firmware "placed" BAR0
extern Backend* g_backend;
extern uint64_t g_clock;                       // fake time-stamp counter
