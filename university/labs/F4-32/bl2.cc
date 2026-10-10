// bl2.cc - F4-32 stage 1 ("BL2", the trusted boot loader). Runs at EL3 from secure RAM. On a real
// SoC this stage initialises DRAM first; QEMU's RAM needs no training, so it only prints that
// step. Then it loads BL31 (resident monitor) and BL33 (the kernel) and starts BL31.
#include "../F4-31/kbase.h"
#include "bootimg.h"

namespace {
constexpr uint64_t kUart = 0x09000000;
constexpr uint64_t kBl31Offset = 0x00020000;
constexpr uint64_t kBl33Offset = 0x00040000;
constexpr uint64_t kDtb = 0x40000000;          // QEMU places its DTB at the start of RAM
void putc_bl2(char c)
{
    while ((k::rd32(kUart + 0x18) & (1u << 5)) != 0) {
    }
    k::wr32(kUart, static_cast<uint8_t>(c));
}
const BootImage* load_or_stop(uint64_t offset)
{
    const auto* img = reinterpret_cast<const BootImage*>(offset);
    const char* err = load_image(img);
    if (err != nullptr) {
        k::printf("BL2 : image at flash offset 0x%lx: %s - stopping\n", offset, err);
        k::exit(6);
    }
    k::printf("BL2 : %s loaded to 0x%x (%u bytes, CRC-32 ok)\n", img->name, img->load, img->size);
    return img;
}
}  // namespace

extern "C" void stage_main()
{
    k::set_console(putc_bl2);
    k::printf("BL2 : running at EL3 from secure RAM; DRAM init would happen here (QEMU: nothing to do)\n");
    const BootImage* bl31 = load_or_stop(kBl31Offset);
    const BootImage* bl33 = load_or_stop(kBl33Offset);
    uint32_t magic = __builtin_bswap32(k::rd32(kDtb));
    k::printf("BL2 : devicetree at 0x%lx: magic 0x%08x (%s)\n", kDtb, magic,
              magic == 0xd00dfeed ? "valid" : "MISSING");
    k::printf("BL2 : starting BL31 with BL33 entry 0x%x and DTB 0x%lx\n", bl33->load, kDtb);
    auto bl31_entry = reinterpret_cast<void (*)(uint64_t, uint64_t)>(static_cast<uintptr_t>(bl31->load));
    bl31_entry(bl33->load, kDtb);
}
