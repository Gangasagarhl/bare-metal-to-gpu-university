// fb.cc - F3-23: find QEMU's "VGA" PCI device and switch it to a 32-bit linear framebuffer.
// Device IDs, the Bochs VBE ("DISPI") I/O ports and register numbers are QEMU/Bochs device
// details written from memory and NOT verified against their documentation (see the chapter's
// unverified box); the lab's screendump shows whether they worked in QEMU 8.2.2.
#include "fb.h"
#include "arch.h"
#include "kprint.h"
#include "paging.h"

namespace fb {
namespace {

constexpr uint16_t kDispiIndex = 0x01CE, kDispiData = 0x01CF;
constexpr uint16_t kRegId = 0, kRegXres = 1, kRegYres = 2, kRegBpp = 3, kRegEnable = 4;
constexpr uint16_t kEnabled = 0x01, kLinearFb = 0x40;
constexpr uint64_t kFbVirt = 0xffffe00000000000;

uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off)
{
    // PCI configuration mechanism #1: address to 0xCF8, data from 0xCFC (see F1-45)
    arch::outl(0xCF8, 0x80000000u | (uint32_t{bus} << 16) | (uint32_t{dev} << 11) | (uint32_t{fn} << 8) | (off & 0xFC));
    return arch::inl(0xCFC);
}

void dispi_write(uint16_t reg, uint16_t value)
{
    asm volatile("outw %0, %1" : : "a"(reg), "Nd"(kDispiIndex));
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(kDispiData));
}

uint16_t dispi_read(uint16_t reg)
{
    uint16_t v;
    asm volatile("outw %0, %1" : : "a"(reg), "Nd"(kDispiIndex));
    asm volatile("inw %1, %0" : "=a"(v) : "Nd"(kDispiData));
    return v;
}

} // namespace

bool init(int width, int height, Info& out)
{
    for (uint8_t dev = 0; dev < 32; ++dev) {
        uint32_t id = pci_read32(0, dev, 0, 0);
        if (id != 0x11111234) {                // device 0x1111, vendor 0x1234 (QEMU's VGA)
            continue;
        }
        uint64_t bar0 = pci_read32(0, dev, 0, 0x10) & ~0xFull;
        kprintf("fb: display device 1234:1111 at 00:%02x.0, BAR0 %08lx, DISPI id %x\n", unsigned{dev},
                bar0, unsigned{dispi_read(kRegId)});
        // PAT entry 4 := write-combining (memory type 0x01), so a page whose PTE has the PAT
        // bit set and PCD = PWT = 0 is write-combining (Intel SDM Vol. 3, "Page Attribute
        // Table"; pending verification). QEMU's TCG does not model memory types at all.
        uint64_t pat = arch::rdmsr(0x277);
        uint64_t new_pat = (pat & ~(0xFFull << 32)) | (0x01ull << 32);
        arch::wrmsr(0x277, new_pat);
        kprintf("fb: PAT MSR %016lx -> %016lx (entry 4 = write-combining)\n", pat, new_pat);
        dispi_write(kRegEnable, 0);
        dispi_write(kRegXres, static_cast<uint16_t>(width));
        dispi_write(kRegYres, static_cast<uint16_t>(height));
        dispi_write(kRegBpp, 32);
        dispi_write(kRegEnable, kEnabled | kLinearFb);
        uint64_t bytes = uint64_t(width) * height * 4;
        for (uint64_t off = 0; off < bytes; off += 4096) {
            // PAT bit set, PCD = PWT = 0: PAT entry 4, write-combining
            if (!paging::kernel_space().map(kFbVirt + off, bar0 + off,
                                            paging::kWrite | paging::kNoExec | paging::kPat4K)) {
                return false;
            }
        }
        out = Info{bar0, reinterpret_cast<uint32_t*>(kFbVirt), width, height, width};
        kprintf("fb: mode %dx%d, 32 bits per pixel, mapped at %p\n", width, height,
                static_cast<void*>(out.pixels));
        return true;
    }
    kprintf("fb: no display device found\n");
    return false;
}

} // namespace fb
