// nvme_ring.cpp - F1-50 Listing 1: submit six commands through a 4-entry SQ/CQ pair,
// watching the doorbells, the "queue full" rule and the phase tag flip at the wrap.
#include "nvme_model.h"

#include <cstdio>

int main()
{
    const int entries = 4;
    Controller ctrl(entries);
    Host host(ctrl, entries);

    std::printf("batch 1\n");
    for (int cid = 1; cid <= 4; ++cid) {
        host.submit(cid);
    }
    host.ring();
    host.poll();

    std::printf("batch 2\n");
    for (int cid = 4; cid <= 6; ++cid) {
        host.submit(cid);
    }
    host.ring();
    host.poll();

    std::printf("CQ memory at the end (slot: cid/phase):");
    for (std::size_t i = 0; i < ctrl.cqMemory().size(); ++i) {
        std::printf(" %zu:%d/%d", i, ctrl.cqMemory()[i].cid, ctrl.cqMemory()[i].phase);
    }
    std::printf("\n");
    return 0;
}
