// aql_sim.h - DR405 F4-42: a user-mode AQL queue and a toy packet processor, on the host.
// The packet layout is the real one from the ROCm header <hsa/hsa.h> (libhsa-runtime-dev
// 5.7.1). The "packet processor" is the university's own model of the idea, not AMD's
// firmware: it shows the protocol (slot, header, write index, doorbell), not the hardware.
#pragma once
#include <hsa/hsa.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace aql {
constexpr uint32_t kSlots = 8;                       // a power of two, as ring indices wrap

inline uint16_t header(hsa_packet_type_t type, bool barrier)
{
    return static_cast<uint16_t>(
        (type << HSA_PACKET_HEADER_TYPE) | ((barrier ? 1 : 0) << HSA_PACKET_HEADER_BARRIER) |
        (HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_SCACQUIRE_FENCE_SCOPE) |
        (HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_SCRELEASE_FENCE_SCOPE));
}
inline unsigned type_of(uint16_t h) { return h & ((1u << HSA_PACKET_HEADER_WIDTH_TYPE) - 1); }

struct Queue {
    std::array<hsa_kernel_dispatch_packet_t, kSlots> ring{};
    uint64_t write_index = 0, read_index = 0;        // in packets, never wrapped
    uint64_t doorbell = UINT64_MAX;                  // last packet id announced
    Queue()
    {
        for (auto& p : ring) p.header = static_cast<uint16_t>(HSA_PACKET_TYPE_INVALID);
    }
    hsa_kernel_dispatch_packet_t& slot(uint64_t id) { return ring[id % kSlots]; }
};

// What one dispatch asks for (a 1-D grid here).
struct Dispatch { uint32_t grid, block; uint64_t kernel_object, kernarg; };

// The toy packet processor: consumes valid packets after the doorbell, and prints what it
// would launch. Real hardware does this in the command processor's firmware.
inline void process(Queue& q, const char* when)
{
    while (q.doorbell != UINT64_MAX && q.read_index <= q.doorbell) {
        hsa_kernel_dispatch_packet_t& p = q.slot(q.read_index);
        if (type_of(p.header) == HSA_PACKET_TYPE_INVALID) break;   // not published yet
        if (type_of(p.header) != HSA_PACKET_TYPE_KERNEL_DISPATCH) {
            std::printf("  [%s] packet %llu: type %u not handled by this model\n", when,
                        static_cast<unsigned long long>(q.read_index), type_of(p.header));
        } else {
            const uint32_t groups = (p.grid_size_x + p.workgroup_size_x - 1) / p.workgroup_size_x;
            std::printf("  [%s] packet %llu: launch kernel_object=0x%llx grid=%u block=%u -> %u workgroups, "
                        "kernarg=0x%llx\n", when, static_cast<unsigned long long>(q.read_index),
                        static_cast<unsigned long long>(p.kernel_object), p.grid_size_x, p.workgroup_size_x,
                        groups, static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(p.kernarg_address)));
        }
        p.header = static_cast<uint16_t>(HSA_PACKET_TYPE_INVALID);   // give the slot back
        ++q.read_index;
    }
}
}  // namespace aql
