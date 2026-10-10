// F1-44 Listing 1: moving 256 words from a device into memory, two ways.
// (a) programmed I/O: the CPU reads each word from the device and stores it;
// (b) DMA: the CPU writes a descriptor (source, destination, length), starts
//     the engine, does other work, and gets a completion interrupt.
// Exercise costs, not a real machine: 1 CPU tick per word moved by the CPU,
// 1 bus tick per word moved by the DMA engine, 5 ticks to write a descriptor,
// 3 ticks for the completion handler.
#include <cstdint>
#include <cstdio>
#include <vector>

struct Descriptor
{
    std::size_t src;      // index in the device's buffer
    std::size_t dst;      // index in memory
    std::size_t len;      // words
    bool done = false;
};

int main()
{
    const std::size_t n = 256;
    std::vector<std::uint32_t> device(n);
    for (std::size_t i = 0; i < n; ++i) {
        device[i] = static_cast<std::uint32_t>(i * 3 + 1);
    }

    // (a) programmed I/O
    std::vector<std::uint32_t> mem_a(1024);
    long cpu_ticks_a = 0;
    for (std::size_t i = 0; i < n; ++i) {
        mem_a[100 + i] = device[i];        // the CPU itself moves every word
        ++cpu_ticks_a;
    }

    // (b) DMA
    std::vector<std::uint32_t> mem_b(1024);
    Descriptor d{0, 100, n};
    long cpu_ticks_b = 5;                  // write the descriptor and start
    long other_work = 0;                   // what the CPU did meanwhile
    std::size_t moved = 0;
    for (long t = 0; !d.done; ++t) {
        mem_b[d.dst + moved] = device[d.src + moved];   // the engine moves one word
        ++moved;
        if (moved == d.len) {
            d.done = true;                 // engine raises "transfer complete"
        }
        ++other_work;                      // the CPU is free during this tick
    }
    cpu_ticks_b += 3;                      // completion handler

    const bool same = mem_a == mem_b;
    std::printf("words moved: %zu\n", n);
    std::printf("PIO: CPU busy %ld ticks moving data, 0 ticks free\n", cpu_ticks_a);
    std::printf("DMA: CPU busy %ld ticks (descriptor + handler), %ld ticks free for other work\n",
                cpu_ticks_b, other_work);
    std::printf("memory contents identical: %s; mem[100]=%u mem[355]=%u\n", same ? "yes" : "NO",
                static_cast<unsigned>(mem_b[100]), static_cast<unsigned>(mem_b[355]));
    return 0;
}
