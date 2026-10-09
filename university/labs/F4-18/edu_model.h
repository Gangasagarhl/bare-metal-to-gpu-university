// edu_model.h - a host model of PCI device 1234:11e8, written from F4-16's observed
// specification (spec_observed.txt), plus switches that inject the faults of the test plan.
// It is a model of what was OBSERVED, not of the device's documentation.
#pragma once
#include <cstdint>

struct EduModel {
    // observed behaviour
    uint32_t id = 0x010000ED, check = 0, compute = 0, irq_status = 0;
    int busy_polls_left = 0;          // STATUS bit 0 stays 1 for this many STATUS reads
    int busy_polls_per_compute = 3;
    // fault switches (test plan, "fault injection")
    bool all_ones = false;            // surprise removal on real PCIe: every read is 0xffffffff
    bool zeros = false;               // QEMU with decoding off: every read is 0
    bool stuck_busy = false;          // STATUS bit 0 never clears
    bool no_invert = false;           // CHECK echoes instead of inverting
    bool ack_ignored = false;         // IRQ_ACK has no effect
    unsigned reads = 0, writes = 0;

    uint32_t read(uint32_t off)
    {
        ++reads;
        if (all_ones) return 0xFFFFFFFFu;
        if (zeros) return 0;
        switch (off) {
        case 0x00: return id;
        case 0x04: return no_invert ? check : ~check;
        case 0x08: return busy_polls_left > 0 ? compute_arg : compute;   // N while busy (observed)
        case 0x20:
            if (stuck_busy) return 1;
            if (busy_polls_left > 0) { --busy_polls_left; return 1; }
            return 0;
        case 0x24: return irq_status;
        default: return 0xFFFFFFFFu;                                 // unimplemented (observed)
        }
    }

    void write(uint32_t off, uint32_t v)
    {
        ++writes;
        if (all_ones || zeros) return;
        switch (off) {
        case 0x04: check = v; break;
        case 0x08: {
            compute_arg = v;
            uint32_t f = 1;
            for (uint32_t i = 2; i <= v; ++i) f *= i;      // n! modulo 2^32, as observed for n <= 14
            compute = f;
            busy_polls_left = busy_polls_per_compute;
            break;
        }
        case 0x60: irq_status |= v; break;                         // observed with one value only:
        case 0x64: if (!ack_ignored) irq_status &= ~v; break;     // OR / AND-NOT is the model's guess
        default: break;
        }
    }

private:
    uint32_t compute_arg = 0;
};
