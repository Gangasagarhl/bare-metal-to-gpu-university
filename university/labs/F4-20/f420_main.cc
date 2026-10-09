// f420_main.cc - F4-20 bring-up kernel: the order a first boot on a new PC follows, one stage at
// a time, each stage stamped with time-stamp counter ticks so a slow or hung stage shows up in
// the log. Stages: console, cpu, memory map, firmware tables (SMBIOS, ACPI), legacy probes,
// PCI binding. A stage that fails prints why and the boot goes on: a bring-up log should show
// everything that can be learned in one boot.
#include "k4.h"
#include "pci4.h"
#include "acpi4.h"
#include "bind4.h"
#include "smbios4.h"

static uint64_t g_t0, g_stage_start;
static int g_bound, g_unbound;

static void stamp()
{
    k4::puts("[tsc+");
    k4::dec(k4::rdtsc() - g_t0);
    k4::puts("] ");
}
static void stage(const char* name)
{
    g_stage_start = k4::rdtsc();
    stamp(); k4::puts("stage "); k4::line(name);
}
static void stage_end(const char* name)
{
    stamp(); k4::puts("end "); k4::puts(name); k4::puts(" ticks ");
    k4::dec(k4::rdtsc() - g_stage_start); k4::putc('\n');
}

static uint32_t rd32(uintptr_t a) { return *reinterpret_cast<const volatile uint32_t*>(a); }
static uint64_t rd64(uintptr_t a) { return rd32(a) | (static_cast<uint64_t>(rd32(a + 4)) << 32); }

// Multiboot (version 1) information, offsets from memory of the Multiboot Specification 0.6.96
// (title only): [0] flags; bit 0 -> [4] mem_lower KiB, [8] mem_upper KiB; bit 6 -> [44]
// mmap_length, [48] mmap_addr; each entry: u32 size (not counting itself), u64 base, u64 length,
// u32 type (1 = usable RAM).
static void memory_map(uint32_t magic, uint32_t info)
{
    if (magic != 0x2BADB002) { k4::line("  not started by a Multiboot loader: no memory map"); return; }
    const uint32_t flags = rd32(info);
    if (flags & 1) {
        k4::puts("  mem_lower KiB "); k4::dec(rd32(info + 4));
        k4::puts(" mem_upper KiB "); k4::dec(rd32(info + 8)); k4::putc('\n');
    }
    if (!(flags & (1u << 6))) { k4::line("  no memory map given (flags bit 6 clear)"); return; }
    const uint32_t len = rd32(info + 44), addr = rd32(info + 48);
    uint64_t usable = 0;
    for (uint32_t p = addr; p < addr + len; p += rd32(p) + 4) {
        const uint64_t base = rd64(p + 4), size = rd64(p + 12);
        const uint32_t type = rd32(p + 20);
        k4::puts("  range 0x"); k4::hex(base, 9); k4::puts(" + 0x"); k4::hex(size, 9);
        k4::puts(type == 1 ? " usable" : " reserved (type "); if (type != 1) { k4::dec(type); k4::putc(')'); }
        k4::putc('\n');
        if (type == 1) usable += size;
    }
    k4::puts("  usable RAM KiB "); k4::dec(usable >> 10); k4::putc('\n');
}

static void smbios_visit(uint8_t type, uint16_t, const uint8_t* s, uint8_t len, const char* str)
{
    if (type == 0 && len > 8) {
        k4::puts("  type 0 firmware: vendor \""); k4::puts(smbios4::string_at(str, s[4]));
        k4::puts("\" version \""); k4::puts(smbios4::string_at(str, s[5]));
        k4::puts("\" date \""); k4::puts(smbios4::string_at(str, s[8])); k4::line("\"");
    } else if (type == 1 && len > 7) {
        k4::puts("  type 1 system: maker \""); k4::puts(smbios4::string_at(str, s[4]));
        k4::puts("\" product \""); k4::puts(smbios4::string_at(str, s[5]));
        k4::puts("\" version \""); k4::puts(smbios4::string_at(str, s[6])); k4::line("\"");
    }
}
static void smbios_type_count(uint8_t type, uint16_t, const uint8_t*, uint8_t, const char*)
{
    k4::puts(" "); k4::dec(type);
}

// A legacy controller probe: the PS/2 keyboard controller (i8042). Its status register is read
// at port 0x64; bit 1 set means "input buffer full: do not send a command yet". A port with no
// device behind it reads 0xFF on this machine, so an absent controller looks permanently busy.
// The register meaning is from memory (see the chapter's unverified box); the 0xFF reading is
// what this lab's own runs print.
static constexpr uint16_t kPs2Status = 0x64;
[[maybe_unused]] static bool ps2_present()
{
    const uint8_t st = k4::inb(kPs2Status);
    k4::puts("  i8042 status port 0x64 reads 0x"); k4::hex(st, 2);
    return st != 0xFF;
}
// Wait until status bit 1 reads 0, at most `limit` ticks. Returns the ticks spent.
static uint64_t ps2_wait_input_empty(uint64_t limit)
{
    const uint64_t t = k4::rdtsc();
    while (k4::rdtsc() - t < limit)
        if (!(k4::inb(kPs2Status) & 0x02)) break;
    return k4::rdtsc() - t;
}

static void bind_one(const pci4::Function& f, void*)
{
    const bind4::Driver* d = bind4::find(f);
    k4::puts(d ? "  bound    " : "  NO DRIVER ");
    k4::hex(f.at.bus, 2); k4::putc(':'); k4::hex(f.at.dev, 2); k4::putc('.'); k4::hex(f.at.fn, 1);
    k4::puts(" "); k4::hex(f.vendor, 4); k4::putc(':'); k4::hex(f.device, 4);
    k4::puts(" class "); k4::hex(f.base_class, 2); k4::putc(' '); k4::hex(f.sub_class, 2);
    if (d) { k4::puts(" -> "); k4::puts(d->name); }
    k4::putc('\n');
    (d ? g_bound : g_unbound)++;
}

extern "C" void kmain(uint32_t magic, uint32_t info)
{
    g_t0 = k4::rdtsc();
    g_stage_start = g_t0;
    k4::console_init();
    stamp(); k4::line("F4-20 bring-up: stage console (first sign of life)");
    stage_end("console");

    stage("cpu");
    {
        const k4::Cpuid v = k4::cpuid(0);
        char vendor[13];
        const uint32_t w[3] = {v.b, v.d, v.c};
        for (int i = 0; i < 12; ++i) vendor[i] = static_cast<char>(w[i / 4] >> (8 * (i % 4)));
        vendor[12] = 0;
        const k4::Cpuid f = k4::cpuid(1);
        k4::puts("  vendor "); k4::puts(vendor); k4::puts(" max leaf "); k4::dec(v.a);
        k4::puts(" signature 0x"); k4::hex(f.a, 8);
        k4::puts((f.c >> 31) & 1 ? " hypervisor bit set" : " hypervisor bit clear"); k4::putc('\n');
    }
    stage_end("cpu");

    stage("memory");
    memory_map(magic, info);
    stage_end("memory");

    stage("smbios");
    {
        smbios4::Entry e;
        if (!smbios4::find(e)) {
            k4::line("  no SMBIOS entry point in 0xF0000..0xFFFFF");
        } else {
            k4::puts(e.v3 ? "  entry _SM3_ at 0x" : "  entry _SM_ at 0x"); k4::hex(e.at, 5);
            k4::puts(" version "); k4::dec(static_cast<uint64_t>(e.major)); k4::putc('.');
            k4::dec(static_cast<uint64_t>(e.minor));
            k4::puts(e.ok ? " checksum ok" : " CHECKSUM BAD"); k4::putc('\n');
            if (e.ok) {
                smbios4::walk(e, smbios_visit);
                k4::puts("  structure types:");
                const int n = smbios4::walk(e, smbios_type_count);
                k4::puts(" ("); k4::dec(static_cast<uint64_t>(n)); k4::line(" structures)");
            }
        }
    }
    stage_end("smbios");

    stage("acpi");
    if (acpi4::init()) {
        k4::puts("  tables:");
        for (int i = 0; i < acpi4::count(); ++i) {
            const acpi4::Table t = acpi4::at(i);
            k4::putc(' '); k4::puts(t.sig); if (!t.checksum_ok) k4::puts("(BAD)");
        }
        k4::putc('\n');
    } else {
        k4::line("  no ACPI tables found");
    }
    stage_end("acpi");

    stage("legacy");
    {
        constexpr uint64_t kWaitLimit = 4000000000ull;   // ticks allowed for "controller ready"
#ifdef F420_NO_PRESENCE_CHECK
        // The version that trusts the controller to be there and waits for it to be ready.
        k4::puts("  i8042: waiting for input buffer empty");
        const uint64_t spent = ps2_wait_input_empty(kWaitLimit);
        k4::puts(", stopped after ticks "); k4::dec(spent); k4::putc('\n');
#else
        if (ps2_present()) {
            k4::puts(" -> present; ready after ticks ");
            k4::dec(ps2_wait_input_empty(kWaitLimit)); k4::putc('\n');
        } else {
            k4::line(" -> absent, skipped");
        }
#endif
    }
    stage_end("legacy");

    stage("pci");
    pci4::enumerate(bind_one, nullptr);
    k4::puts("  functions bound "); k4::dec(static_cast<uint64_t>(g_bound));
    k4::puts(", without a driver "); k4::dec(static_cast<uint64_t>(g_unbound)); k4::putc('\n');
    stage_end("pci");

    stamp(); k4::line("bring-up complete");
    k4::exit_qemu(0x10);
}
