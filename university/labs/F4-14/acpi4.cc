// acpi4.cc - ACPI table discovery for the DR401 lab kernel (read only).
// Layouts used (all to be checked against the ACPI Specification, title only in this build):
// the RSDP starts with "RSD PTR " on a 16-byte boundary and holds the RSDT address at byte 16;
// every system description table starts with a 36-byte header (signature, length at byte 4);
// the RSDT body is a list of 32-bit table addresses; the FADT ("FACP") holds the DSDT address at
// byte 40. Each table's bytes must sum to zero modulo 256: the kernel checks that for every
// table it reports, which is how a wrong offset would show up.
#include "acpi4.h"
#include "k4.h"

namespace acpi4 {

static uintptr_t g_rsdt = 0;

// The empty asm hides the constant address from the optimiser, which otherwise warns about
// reading "outside an object" at low physical addresses such as 0x40E.
static uintptr_t opaque(uintptr_t a) { asm("" : "+r"(a)); return a; }
static uint32_t rd32(uintptr_t a) { return *reinterpret_cast<const volatile uint32_t*>(opaque(a)); }
static uint16_t rd16(uintptr_t a) { return *reinterpret_cast<const volatile uint16_t*>(opaque(a)); }
static uint8_t rd8(uintptr_t a) { return *reinterpret_cast<const volatile uint8_t*>(opaque(a)); }

static bool sum_ok(uintptr_t a, uint32_t len)
{
    uint8_t s = 0;
    for (uint32_t i = 0; i < len; ++i) s = static_cast<uint8_t>(s + rd8(a + i));
    return s == 0;
}

static bool is_rsdp(uintptr_t a)
{
    const char* sig = "RSD PTR ";
    for (int i = 0; i < 8; ++i)
        if (rd8(a + i) != static_cast<uint8_t>(sig[i])) return false;
    return sum_ok(a, 20);   // the first 20 bytes carry the original checksum
}

bool init()
{
    const uintptr_t ebda = uintptr_t{rd16(0x40E)} << 4;   // BIOS data area: EBDA segment
    uintptr_t found = 0;
    for (uintptr_t a = ebda; ebda && a < ebda + 1024 && !found; a += 16)
        if (is_rsdp(a)) found = a;
    for (uintptr_t a = 0xE0000; a < 0x100000 && !found; a += 16)
        if (is_rsdp(a)) found = a;
    if (!found) return false;
    g_rsdt = rd32(found + 16);
    k4::puts("acpi: RSDP at 0x"); k4::hex(found, 8);
    k4::puts(", RSDT at 0x"); k4::hex(g_rsdt, 8); k4::putc('\n');
    return true;
}

static Table table_at(uintptr_t a)
{
    Table t{};
    t.addr = a;
    t.length = rd32(a + 4);
    for (int i = 0; i < 4; ++i) t.sig[i] = static_cast<char>(rd8(a + i));
    t.sig[4] = 0;
    t.checksum_ok = sum_ok(a, t.length);
    return t;
}

int count() { return g_rsdt ? static_cast<int>((rd32(g_rsdt + 4) - 36) / 4) : 0; }

Table at(int i) { return table_at(rd32(g_rsdt + 36 + 4 * static_cast<uint32_t>(i))); }

bool find(const char* sig, Table& t)
{
    for (int i = 0; i < count(); ++i) {
        Table c = at(i);
        if (c.sig[0] == sig[0] && c.sig[1] == sig[1] && c.sig[2] == sig[2] && c.sig[3] == sig[3]) {
            t = c;
            return true;
        }
    }
    return false;
}

bool dsdt(Table& t)
{
    Table fadt;
    if (!find("FACP", fadt)) return false;
    t = table_at(rd32(fadt.addr + 40));
    return t.sig[0] == 'D' && t.sig[1] == 'S' && t.sig[2] == 'D' && t.sig[3] == 'T';
}

void dump_hex(const Table& t, const char* tag)
{
    for (uint32_t off = 0; off < t.length; off += 32) {
        k4::puts(tag); k4::putc(' '); k4::hex(off, 5); k4::puts(":");
        for (uint32_t i = off; i < off + 32 && i < t.length; ++i) {
            k4::putc(' ');
            k4::hex(rd8(t.addr + i), 2);
        }
        k4::putc('\n');
    }
}

} // namespace acpi4
