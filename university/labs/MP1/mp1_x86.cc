// mp1_x86.cc - MP1 starter: the x86-64 port of the regression kernel.
// Boot path reused unchanged from F3-18 (Multiboot v1 through QEMU -kernel, boot.S, serial,
// kprint, panic, cxxrt). The CPU count comes from the ACPI MADT, parsed by F3-24's madt.h
// (pure logic, unchanged) after a small RSDP/RSDT walk written here with F3-24's acpi.h
// header layout. All table layouts carry F3-24's unverified box (ACPI Specification).
#include <cstdint>
#include "acpi.h"
#include "arch.h"
#include "kprint.h"
#include "madt.h"
#include "mp1_core.h"
#include "multiboot.h"
#include "serial.h"

namespace {

bool sig_is(const void* p, const char* sig, int n)
{
    const auto* b = static_cast<const char*>(p);
    for (int i = 0; i < n; ++i) {
        if (b[i] != sig[i]) {
            return false;
        }
    }
    return true;
}

uint32_t rd32(const uint8_t* p)   // byte by byte: table fields need not be aligned
{
    return uint32_t{p[0]} | uint32_t{p[1]} << 8 | uint32_t{p[2]} << 16 | uint32_t{p[3]} << 24;
}

// F3-18's boot page tables map physical 0..8 GiB at the direct-map base as well (PML4[256]),
// so firmware tables are read there, the way F3-24 does it through pmm::phys_to_virt.
constexpr uint64_t kDirectMap = 0xffff800000000000;
const uint8_t* phys(uint64_t a) { return reinterpret_cast<const uint8_t*>(kDirectMap + a); }

const uint8_t* find_rsdp()
{
    uint64_t ebda = uint64_t{*reinterpret_cast<const volatile uint16_t*>(phys(0x40e))} << 4;
    const uint64_t ranges[2][2] = {{ebda, ebda + 1024}, {0xe0000, 0x100000}};
    for (const auto& r : ranges) {
        for (uint64_t a = r[0]; a + 20 <= r[1]; a += 16) {
            if (r[0] != 0 && sig_is(phys(a), "RSD PTR ", 8) && acpi::checksum_ok(phys(a), 20)) {
                return phys(a);
            }
        }
    }
    return nullptr;
}

int count_cpus_madt()
{
    const uint8_t* rsdp = find_rsdp();
    if (rsdp == nullptr) {
        return -1;
    }
    const uint8_t* rsdt = phys(rd32(rsdp + 16));        // RSDT address (32-bit) in every revision
    auto* h = reinterpret_cast<const acpi::Header*>(rsdt);
    if (!sig_is(h->signature, "RSDT", 4) || !acpi::checksum_ok(rsdt, h->length)) {
        return -1;
    }
    for (uint32_t off = sizeof(acpi::Header); off + 4 <= h->length; off += 4) {
        const uint8_t* t = phys(rd32(rsdt + off));
        auto* th = reinterpret_cast<const acpi::Header*>(t);
        if (sig_is(th->signature, "APIC", 4) && acpi::checksum_ok(t, th->length)) {
            static madt::Info info;                     // large: keep it off the boot stack
            info = madt::Info{};
            if (!madt::parse(t, th->length, info)) {
                return -1;
            }
            int enabled = 0;
            for (int i = 0; i < info.ncpu; ++i) {
                enabled += info.cpus[i].enabled ? 1 : 0;
            }
            kprintf("acpi: RSDP at physical 0x%lx, MADT %u bytes, %d processor entries, "
                    "%d enabled\n", static_cast<unsigned long>(rsdp - phys(0)), th->length,
                    info.ncpu, enabled);
            return enabled;
        }
    }
    return -1;
}

uint64_t now() { return arch::rdtsc(); }

} // namespace

extern "C" void kmain(uint32_t magic, uint32_t info_phys)
{
    uint64_t t0 = arch::rdtsc();
    serial::init();
    kprintf("MP1 starter kernel, x86-64 port (F3-18 boot path)\n");
    const char* args = "";
    if (magic == mb::kBootMagic) {
        auto* info = reinterpret_cast<const mb::Info*>(uintptr_t{info_phys});
        if ((info->flags & mb::kFlagCmdline) && info->cmdline != 0) {
            args = reinterpret_cast<const char*>(uintptr_t{info->cmdline});
        }
    }
    mp1::Platform p{
        "x86_64", "multiboot1", nullptr, 4096, args, t0, 0, now, count_cpus_madt,
        nullptr,   // storage: not in the starter; the NVMe driver of F4-07 moves in at milestone 3
    };
    int failures = mp1::run(p);
    arch::qemu_exit(failures == 0 ? arch::kExitPass : arch::kExitFail);
}
