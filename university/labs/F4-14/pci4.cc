// pci4.cc - PCI configuration access and enumeration for the DR401 lab kernel.
// Register offsets and bit names: Linux UAPI <linux/pci_regs.h> (linux-libc-dev, this build).
// The CONFIG_ADDRESS layout (enable bit 31, bus 23:16, device 15:11, function 10:8, register 7:2)
// is configuration mechanism #1 of the PCI Local Bus Specification (title only in this build);
// F4-14's run.sh checks the result against QEMU's own list of its PCI devices.
#include "pci4.h"
#include "k4.h"
#include <linux/pci_regs.h>

namespace pci4 {

static constexpr uint16_t kConfigAddress = 0xCF8;
static constexpr uint16_t kConfigData = 0xCFC;

static uint32_t address(Addr a, uint8_t off)
{
    return 0x80000000u | (uint32_t{a.bus} << 16) | (uint32_t{a.dev} << 11) |
           (uint32_t{a.fn} << 8) | (off & 0xFCu);
}

uint32_t read32(Addr a, uint8_t off)
{
    k4::outl(kConfigAddress, address(a, off));
    return k4::inl(kConfigData);
}

void write32(Addr a, uint8_t off, uint32_t v)
{
    k4::outl(kConfigAddress, address(a, off));
    k4::outl(kConfigData, v);
}

uint16_t read16(Addr a, uint8_t off) { return static_cast<uint16_t>(read32(a, off) >> ((off & 2) * 8)); }
uint8_t read8(Addr a, uint8_t off) { return static_cast<uint8_t>(read32(a, off) >> ((off & 3) * 8)); }

bool read_function(Addr a, Function& f)
{
    const uint16_t vendor = read16(a, PCI_VENDOR_ID);
    if (vendor == 0xFFFF) return false;               // nobody answered: reads return all ones
    const uint32_t cr = read32(a, PCI_CLASS_REVISION);  // class in bits 31:8, revision in 7:0
    f.at = a;
    f.vendor = vendor;
    f.device = read16(a, PCI_DEVICE_ID);
    f.revision = static_cast<uint8_t>(cr);
    f.prog_if = static_cast<uint8_t>(cr >> 8);
    f.sub_class = static_cast<uint8_t>(cr >> 16);
    f.base_class = static_cast<uint8_t>(cr >> 24);
    f.header_type = read8(a, PCI_HEADER_TYPE);
    if ((f.header_type & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_NORMAL) {
        f.subvendor = read16(a, PCI_SUBSYSTEM_VENDOR_ID);
        f.subdevice = read16(a, PCI_SUBSYSTEM_ID);
    } else {
        f.subvendor = f.subdevice = 0;                // bridges keep other registers there
    }
    return true;
}

int enumerate(Visitor visit, void* ctx)
{
    int count = 0;
    for (int bus = 0; bus < 256; ++bus) {
        for (uint8_t dev = 0; dev < 32; ++dev) {
            Function f0;
            if (!read_function(Addr{static_cast<uint8_t>(bus), dev, 0}, f0)) continue;
            visit(f0, ctx);
            ++count;
#ifdef F414_FUNC0_ONLY
            continue;   // the forensic build: "function 0 is the device" (wrong)
#endif
            if (!(f0.header_type & PCI_HEADER_TYPE_MFD)) continue;   // single-function device
            for (uint8_t fn = 1; fn < 8; ++fn) {
                Function f;
                if (read_function(Addr{static_cast<uint8_t>(bus), dev, fn}, f)) {
                    visit(f, ctx);
                    ++count;
                }
            }
        }
    }
    return count;
}

Bar read_bar(Addr a, int i)
{
    const uint8_t off = static_cast<uint8_t>(PCI_BASE_ADDRESS_0 + 4 * i);
    const uint32_t orig = read32(a, off);
    Bar b{};
    b.io = (orig & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO;
    b.is64 = !b.io && (orig & PCI_BASE_ADDRESS_MEM_TYPE_MASK) == PCI_BASE_ADDRESS_MEM_TYPE_64;
    const uint32_t mask = b.io ? PCI_BASE_ADDRESS_IO_MASK : PCI_BASE_ADDRESS_MEM_MASK;
    b.base = orig & mask;
    // Size probe: write all ones, read back which address bits stick, restore. Decoding is
    // switched off meanwhile so the device never answers at a half-written address.
    const uint16_t cmd = read16(a, PCI_COMMAND);
    write32(a, PCI_COMMAND, cmd & ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY));
    write32(a, off, 0xFFFFFFFFu);
    const uint32_t probe = read32(a, off) & mask;
    write32(a, off, orig);
    write32(a, PCI_COMMAND, cmd);
    b.size = probe ? (~probe + 1) & (b.io ? 0xFFFFu : 0xFFFFFFFFu) : 0;
    return b;
}

} // namespace pci4
