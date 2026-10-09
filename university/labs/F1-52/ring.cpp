// ring.cpp - F1-52 Listing 1: a driver and a toy NIC sharing an 8-slot receive ring.
// Legend in the ring line: N = slot owned by the NIC, D = owned by the driver;
// + = empty buffer attached, * = packet received (done), . = no buffer.
#include "ring_model.h"

#include <cstdio>

int main()
{
    const int slots = 8;
    ToyNic nic(slots);
    int nextBuffer = 100;

    // Driver start-up: attach a buffer to every slot, then hand all but one to the NIC
    // (if tail could equal head after handing over every slot, "full" and "empty" would
    // look the same).
    for (int i = 0; i < slots; ++i) {
        nic.ring()[static_cast<std::size_t>(i)].buffer = nextBuffer++;
    }
    nic.writeTail(slots - 1);
    std::printf("after start-up (7 buffers given to the NIC):\n");
    showRing(nic);

    int driverNext = 0;   // the next slot the driver will look at
    auto service = [&](const char* when) {
        int handled = 0;
        while (nic.ring()[static_cast<std::size_t>(driverNext)].done) {
            RxDescriptor& d = nic.ring()[static_cast<std::size_t>(driverNext)];
            std::printf("  driver: slot %d has a %d-byte packet in buffer %d\n",
                        driverNext, d.length, d.buffer);
            d.done = false;
            d.buffer = nextBuffer++;               // a fresh buffer for the next packet
            driverNext = (driverNext + 1) % slots;
            ++handled;
        }
        // Give the refilled slots back: the tail moves to the slot before driverNext.
        nic.writeTail((driverNext + slots - 1) % slots);
        std::printf("%s: driver handled %d packet(s), wrote tail = %d\n", when, handled, nic.tail());
        showRing(nic);
    };

    for (int p = 0; p < 5; ++p) {
        nic.receive(60 + p);
    }
    std::printf("5 packets arrived:\n");
    showRing(nic);
    service("interrupt 1");

    for (int p = 0; p < 9; ++p) {
        nic.receive(70 + p);
    }
    std::printf("9 more packets arrived before the driver ran again:\n");
    showRing(nic);
    service("interrupt 2");
    return 0;
}
