// rx_stall.cpp - F1-52 forensic evidence generator: "the network dies after a few seconds".
// The driver below processes received packets but forgets one step. Output: the counters
// a monitoring script printed once per second, then a register dump, in the simulator's
// own format.
#include "ring_model.h"

#include <cstdio>

int main()
{
    const int slots = 8;
    ToyNic nic(slots);
    for (int i = 0; i < slots; ++i) {
        nic.ring()[static_cast<std::size_t>(i)].buffer = 100 + i;
    }
    nic.writeTail(slots - 1);

    int driverNext = 0;
    long delivered = 0;
    std::printf("time  rx_delivered  rx_missed_no_descriptor  head  tail\n");
    for (int second = 1; second <= 6; ++second) {
        for (int p = 0; p < 3; ++p) {          // three packets arrive every second
            nic.receive(64);
        }
        while (nic.ring()[static_cast<std::size_t>(driverNext)].done) {
            RxDescriptor& d = nic.ring()[static_cast<std::size_t>(driverNext)];
            d.done = false;                    // packet passed up the network stack
            d.buffer += slots;                 // new buffer attached ...
            driverNext = (driverNext + 1) % slots;
            ++delivered;
            // BUG: the driver never writes the tail register again after start-up.
        }
        std::printf("%3ds %10ld %18ld %14d %5d\n", second, delivered, nic.missed(), nic.head(), nic.tail());
    }
    std::printf("\nregister dump: RX_HEAD=%d RX_TAIL=%d RX_RING_SLOTS=%d\n", nic.head(), nic.tail(), slots);
    std::printf("driver state:  next slot to check=%d, buffers attached in all %d slots\n", driverNext, slots);
    return 0;
}
