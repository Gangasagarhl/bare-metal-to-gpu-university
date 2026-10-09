// aql_forensic.cpp - DR405 F4-42 forensic lab: "the dispatch that ran with old numbers".
// Two host threads of an application share one queue (a multi-producer queue). Producer P1
// reserves packet 8, producer P2 packet 9. P1's code (enqueue_header_first) stores the
// header before the body. The interleaving below is fixed so the run is repeatable: P1
// stores its header, P2 publishes packet 9 correctly and rings the doorbell, the packet
// processor runs, and only then does P1 store the body. The log is the evidence pack.
#include "aql_sim.h"

namespace {
void body(hsa_kernel_dispatch_packet_t& p, const aql::Dispatch& d)
{
    p.setup = 1 << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
    p.workgroup_size_x = static_cast<uint16_t>(d.block);
    p.workgroup_size_y = p.workgroup_size_z = 1;
    p.grid_size_x = d.grid;
    p.grid_size_y = p.grid_size_z = 1;
    p.kernel_object = d.kernel_object;
    p.kernarg_address = reinterpret_cast<void*>(static_cast<uintptr_t>(d.kernarg));
}
void log_request(const char* who, uint64_t id, const aql::Dispatch& d)
{
    std::printf("%s: request packet %llu: kernel_object=0x%llx grid=%u block=%u kernarg=0x%llx\n", who,
                static_cast<unsigned long long>(id), static_cast<unsigned long long>(d.kernel_object), d.grid,
                d.block, static_cast<unsigned long long>(d.kernarg));
}
}  // namespace

int main()
{
    aql::Queue q;
    std::printf("== warm-up: packets 0-7 fill every slot of the 8-slot ring once ==\n");
    for (uint32_t i = 0; i < aql::kSlots; ++i) {
        const aql::Dispatch d{4096u * (i + 1), 256, 0x7f00a000 + 0x100ull * i, 0x7f00b000 + 0x40ull * i};
        const uint64_t id = q.write_index++;
        body(q.slot(id), d);
        q.slot(id).header = aql::header(HSA_PACKET_TYPE_KERNEL_DISPATCH, true);
        q.doorbell = id;
        aql::process(q, "pp");
    }
    std::printf("\n== incident window ==\n");
    const uint64_t id8 = q.write_index++, id9 = q.write_index++;
    const aql::Dispatch d8{3000, 64, 0x7f00f000, 0x7f00e000}, d9{128, 128, 0x7f00f100, 0x7f00e040};
    log_request("P1", id8, d8);
    log_request("P2", id9, d9);
    q.slot(id8).header = aql::header(HSA_PACKET_TYPE_KERNEL_DISPATCH, true);   // P1: header first
    std::printf("P1: header of packet %llu stored\n", static_cast<unsigned long long>(id8));
    body(q.slot(id9), d9);                                                   // P2: body first
    q.slot(id9).header = aql::header(HSA_PACKET_TYPE_KERNEL_DISPATCH, true);
    q.doorbell = id9;
    std::printf("P2: packet %llu published, doorbell <- %llu\n", static_cast<unsigned long long>(id9),
                static_cast<unsigned long long>(id9));
    aql::process(q, "pp");
    body(q.slot(id8), d8);                                                   // P1: body last
    std::printf("P1: body of packet %llu stored (too late)\n", static_cast<unsigned long long>(id8));
    return 0;
}
