// Listing 1 (F2-42): one producer, one consumer, 1 000 000 items through SpscRing,
// checked to arrive complete and in order.
#include <cstdint>
#include <iostream>
#include <thread>

#include "spsc_ring.hpp"

int main()
{
    const std::uint32_t items = 1'000'000;
    SpscRing<std::uint32_t> ring(1024);
    std::uint64_t fullSpins = 0;
    std::uint64_t emptySpins = 0;
    bool inOrder = true;
    std::uint32_t received = 0;

    std::thread producer([&] {
        for (std::uint32_t v = 0; v < items; ++v) {
            while (!ring.tryPush(v)) {
                ++fullSpins;  // written only by the producer thread
            }
        }
    });
    std::thread consumer([&] {
        while (received < items) {
            if (auto v = ring.tryPop()) {
                inOrder = inOrder && (*v == received);
                ++received;
            } else {
                ++emptySpins;  // written only by the consumer thread
            }
        }
    });
    producer.join();
    consumer.join();
    std::cout << "received " << received << " of " << items << ", in order: " << (inOrder ? "yes" : "NO") << '\n';
    std::cout << (received == items && inOrder ? "PASS" : "FAIL") << '\n';
    std::cout << "(the producer found the ring full " << (fullSpins > 0 ? "at least once" : "never")
              << "; the consumer found it empty " << (emptySpins > 0 ? "at least once" : "never") << ")\n";
    return received == items && inOrder ? 0 : 1;
}
