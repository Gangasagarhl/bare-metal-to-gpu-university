// F1-47 Listing 3: SPI is two shift registers joined in a ring. On every clock
// the controller shifts one bit out on MOSI while the target shifts one bit out
// on MISO, so every transfer is a swap. In this example both sides send the
// most significant bit first; real devices state their bit order and clock
// mode in their datasheets.
#include <cstdint>
#include <cstdio>

int main()
{
    std::uint8_t ctrl = 0x9F;      // what the controller sends (an example command byte)
    std::uint8_t target = 0xC2;    // what the target had loaded in its shift register
    std::printf("CS low (target selected)\n");
    std::printf("clock  MOSI MISO   controller  target\n");
    for (int clk = 1; clk <= 8; ++clk) {
        const int mosi = (ctrl >> 7) & 1;
        const int miso = (target >> 7) & 1;
        ctrl = static_cast<std::uint8_t>((ctrl << 1) | miso);
        target = static_cast<std::uint8_t>((target << 1) | mosi);
        std::printf("  %d     %d    %d      0x%02X        0x%02X\n", clk, mosi, miso, ctrl, target);
    }
    std::printf("CS high. controller received 0x%02X, target received 0x%02X\n", ctrl, target);
    return 0;
}
