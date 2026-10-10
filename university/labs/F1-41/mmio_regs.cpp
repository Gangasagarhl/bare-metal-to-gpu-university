// F1-41 Listing 1: talking to a device through memory-mapped registers.
// There is no real device here: a small array plays the device's register
// block, and device_tick() plays the hardware changing it on its own. The
// accessors are the pattern drivers use: one volatile access per call, the
// width fixed by the type, read-modify-write that keeps the other bits.
#include <array>
#include <cstdint>
#include <cstdio>

// Register offsets (in 32-bit words) of our made-up device, "TOYDEV".
constexpr std::size_t kCtrl = 0;    // read/write: bit 0 = enable, bit 1 = irq enable
constexpr std::size_t kStatus = 1;  // read-only for software: bit 0 = data ready
constexpr std::size_t kData = 2;    // read: next received byte
constexpr std::size_t kIrqAck = 3;  // write 1 to clear the matching pending bit

std::array<std::uint32_t, 4> g_regs{};   // the simulated register block

std::uint32_t reg_read(std::size_t off)
{
    volatile std::uint32_t* base = g_regs.data();
    return base[off];                   // exactly one 32-bit load
}

void reg_write(std::size_t off, std::uint32_t v)
{
    volatile std::uint32_t* base = g_regs.data();
    base[off] = v;                      // exactly one 32-bit store
}

void reg_set_bits(std::size_t off, std::uint32_t mask)
{
    reg_write(off, reg_read(off) | mask);   // keep every bit we do not own
}

// The "hardware": after it is enabled, a byte arrives on every third tick.
void device_tick(int t)
{
    if ((g_regs[kCtrl] & 1u) != 0 && t % 3 == 0) {
        g_regs[kData] = static_cast<std::uint32_t>('A' + t / 3);
        g_regs[kStatus] |= 1u;          // data ready
    }
    if (g_regs[kIrqAck] & 1u) {         // write-1-to-clear: hardware clears the bit
        g_regs[kStatus] &= ~1u;
        g_regs[kIrqAck] = 0;
    }
}

int main()
{
    g_regs[kCtrl] = 0x80000000u;        // a bit set by "firmware" that we must keep
    reg_set_bits(kCtrl, 0x1u);          // enable the device, keep bit 31
    std::printf("CTRL after enable = 0x%08X (bit 31 kept, bit 0 set)\n",
                static_cast<unsigned>(reg_read(kCtrl)));
    int got = 0;
    for (int t = 1; t <= 12 && got < 3; ++t) {
        device_tick(t);
        const std::uint32_t st = reg_read(kStatus);
        std::printf("tick %2d: STATUS=0x%X", t, static_cast<unsigned>(st));
        if (st & 1u) {
            const auto byte = static_cast<char>(reg_read(kData));
            reg_write(kIrqAck, 1u);     // acknowledge: write 1 to clear "ready"
            ++got;
            std::printf("  DATA='%c'  -> wrote 1 to IRQ_ACK", byte);
        }
        std::printf("\n");
    }
    std::printf("received %d bytes\n", got);
    return 0;
}
