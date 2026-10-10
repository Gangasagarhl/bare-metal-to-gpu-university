// F1-44 Listing 2: why drivers clean and invalidate caches around DMA on a
// non-coherent system. Buffer: bytes 0..31 of memory (two cache lines).
#include <vector>

#include "cache_model.h"

void transmit(bool clean_first)
{
    System s;
    for (std::size_t i = 0; i < 32; ++i) s.cpu_write(i, static_cast<std::uint8_t>(0xA0 + i));
    if (clean_first) s.clean(0, 32);
    std::vector<std::uint8_t> sent(32);
    for (std::size_t i = 0; i < 32; ++i) sent[i] = s.dma_read(i);   // the device reads
    dump(clean_first ? "TX, clean before DMA:" : "TX, no cache clean:", sent.data(), 32);
}

void receive(bool invalidate_first)
{
    System s;
    for (std::size_t i = 0; i < 32; ++i) s.cpu_read(i);             // CPU looked earlier
    for (std::size_t i = 0; i < 32; ++i) s.dma_write(i, static_cast<std::uint8_t>(0x50 + i));
    if (invalidate_first) s.invalidate(0, 32);
    std::vector<std::uint8_t> got(32);
    for (std::size_t i = 0; i < 32; ++i) got[i] = s.cpu_read(i);
    dump(invalidate_first ? "RX, invalidate first:" : "RX, no invalidate:", got.data(), 32);
}

int main()
{
    std::printf("CPU wrote A0..BF for the device; the device wrote 50..6F for the CPU\n");
    transmit(false);
    transmit(true);
    receive(false);
    receive(true);
    return 0;
}
