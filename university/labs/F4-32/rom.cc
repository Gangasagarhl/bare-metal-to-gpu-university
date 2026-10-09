// rom.cc - F4-32 stage 0, the "boot ROM". Runs at EL3 from address 0 (flash in QEMU's virt
// machine with secure=on; mask ROM inside a real SoC). Its only jobs: say hello on the debug
// UART, find the next stage on the boot medium, check it, copy it to on-chip RAM, jump.
#include "../F4-31/kbase.h"
#include "bootimg.h"

namespace {
constexpr uint64_t kUart = 0x09000000;        // a ROM knows its own SoC: fixed addresses are fine
constexpr uint64_t kBl2Offset = 0x00010000;   // where this "SoC" looks for BL2 on the medium
void putc_rom(char c)
{
    while ((k::rd32(kUart + 0x18) & (1u << 5)) != 0) {
    }
    k::wr32(kUart, static_cast<uint8_t>(c));
}
uint64_t current_el()
{
    uint64_t v;
    asm volatile("mrs %0, CurrentEL" : "=r"(v));
    return v >> 2;
}
}  // namespace

extern "C" void stage_main()
{
    k::set_console(putc_rom);
    k::printf("ROM : running at EL%lu from 0x0, boot medium = flash\n", current_el());
    const auto* img = reinterpret_cast<const BootImage*>(kBl2Offset);
    const char* err = load_image(img);
    if (err != nullptr) {
        k::printf("ROM : %s at 0x%lx; no recovery mode in this ROM - stopping\n", err, kBl2Offset);
        k::exit(5);
    }
    k::printf("ROM : %s loaded to 0x%x (%u bytes, CRC-32 0x%08x ok), jumping\n", img->name, img->load,
              img->size, img->crc32);
    auto entry = reinterpret_cast<void (*)()>(static_cast<uintptr_t>(img->load));
    entry();
}
