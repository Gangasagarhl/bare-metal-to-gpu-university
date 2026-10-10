// aql_queue.cpp - DR405 F4-42: build AQL kernel-dispatch packets the way the ROCm runtime
// does after a HIP launch, publish them, ring the doorbell, and let the toy packet
// processor (aql_sim.h) consume them. Also prints the packet layout from <hsa/hsa.h>.
#include <cstddef>
#include "aql_sim.h"

namespace {
// Fill every field except the header, then publish the header last: until then the
// slot still says INVALID and a packet processor must not touch it.
uint64_t enqueue(aql::Queue& q, const aql::Dispatch& d)
{
    const uint64_t id = q.write_index++;              // reserve a slot
    hsa_kernel_dispatch_packet_t& p = q.slot(id);
    p.setup = 1 << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;  // 1-D grid
    p.workgroup_size_x = static_cast<uint16_t>(d.block);
    p.workgroup_size_y = p.workgroup_size_z = 1;
    p.grid_size_x = d.grid;
    p.grid_size_y = p.grid_size_z = 1;
    p.private_segment_size = 0;
    p.group_segment_size = 0;
    p.kernel_object = d.kernel_object;
    p.kernarg_address = reinterpret_cast<void*>(static_cast<uintptr_t>(d.kernarg));
    p.completion_signal = hsa_signal_t{0};
    // On real hardware this store must be a single 16-bit (or 32-bit, with setup) atomic
    // store with release ordering, so the GPU never sees a valid header before the body.
    p.header = aql::header(HSA_PACKET_TYPE_KERNEL_DISPATCH, true);
    return id;
}
}  // namespace

int main()
{
    std::printf("hsa_kernel_dispatch_packet_t: %zu bytes\n", sizeof(hsa_kernel_dispatch_packet_t));
#define F(field) std::printf("  offset %2zu  %s\n", offsetof(hsa_kernel_dispatch_packet_t, field), #field)
    F(header); F(setup); F(workgroup_size_x); F(workgroup_size_y); F(workgroup_size_z); F(grid_size_x);
    F(grid_size_y); F(grid_size_z); F(private_segment_size); F(group_segment_size); F(kernel_object);
    F(kernarg_address); F(completion_signal);
#undef F
    const uint16_t h = aql::header(HSA_PACKET_TYPE_KERNEL_DISPATCH, true);
    std::printf("header for KERNEL_DISPATCH, barrier, system-scope fences: 0x%04x\n\n", h);

    aql::Queue q;
    const aql::Dispatch a{1u << 20, 256, 0x7f00a000, 0x7f00b000}, b{1000, 64, 0x7f00a100, 0x7f00b040};
    std::printf("enqueue A and B, then ring the doorbell once with the last id:\n");
    enqueue(q, a);
    aql::process(q, "before doorbell");               // nothing: the doorbell was not rung
    const uint64_t last = enqueue(q, b);
    q.doorbell = last;                                // the doorbell write
    aql::process(q, "after doorbell");
    std::printf("write_index=%llu read_index=%llu\n\n", static_cast<unsigned long long>(q.write_index),
                static_cast<unsigned long long>(q.read_index));

    std::printf("wrap-around: 10 more dispatches through the %u-slot ring\n", aql::kSlots);
    for (uint32_t i = 0; i < 10; ++i) {
        q.doorbell = enqueue(q, aql::Dispatch{64 * (i + 1), 64, 0x7f00a200, 0x7f00c000 + 0x40ull * i});
        aql::process(q, "doorbell");
    }
    std::printf("write_index=%llu read_index=%llu (slot of the last packet: %llu)\n",
                static_cast<unsigned long long>(q.write_index), static_cast<unsigned long long>(q.read_index),
                static_cast<unsigned long long>((q.write_index - 1) % aql::kSlots));
    return 0;
}
