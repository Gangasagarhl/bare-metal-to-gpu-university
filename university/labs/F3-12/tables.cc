// tables.cc - the tables the firmware gives the OS: the UEFI configuration table, the ACPI
// RSDP and XSDT with a few tables decoded (MADT, HPET, MCFG, FADT), and the SMBIOS 3 structures.
// A UEFI application (same build route as F3-10); it reads memory only and changes nothing.
// Table layouts were written from memory of the ACPI and SMBIOS specifications and are
// checked here only by checksums and by comparison with QEMU's configuration (see F3-12).
#include "console.hpp"
#include "efi.hpp"

namespace {

uint8_t u8(const uint8_t* p, int o) { return p[o]; }
uint16_t u16(const uint8_t* p, int o) { return static_cast<uint16_t>(p[o] | p[o + 1] << 8); }
uint32_t u32(const uint8_t* p, int o) { return u16(p, o) | static_cast<uint32_t>(u16(p, o + 2)) << 16; }
uint64_t u64(const uint8_t* p, int o) { return u32(p, o) | static_cast<uint64_t>(u32(p, o + 4)) << 32; }

const uint8_t* ptr(uint64_t physical)  // firmware memory is identity-mapped while boot services run
{
    return reinterpret_cast<const uint8_t*>(physical);
}

bool sums_to_zero(const uint8_t* p, uint32_t length)
{
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; ++i) {
        sum = static_cast<uint8_t>(sum + p[i]);
    }
    return sum == 0;
}

void chars(Console& con, const uint8_t* p, int n)  // fixed-length ASCII field
{
    char buf[16] = {};
    for (int i = 0; i < n && i < 15; ++i) {
        buf[i] = (p[i] >= 32 && p[i] < 127) ? static_cast<char>(p[i]) : '.';
    }
    con.print(buf);
}

void header(Console& con, const uint8_t* t)  // the 36-byte header every ACPI table starts with
{
    chars(con, t, 4);
    con.print("  length ");
    con.dec(u32(t, 4));
    con.print("  revision ");
    con.dec(u8(t, 8));
    con.print("  OEM '");
    chars(con, t + 10, 6);
    con.print("'  checksum ");
    con.print(sums_to_zero(t, u32(t, 4)) ? "ok" : "BAD");
    con.print("\n");
}

void madt(Console& con, const uint8_t* t)
{
    con.print("    local APIC address ");
    con.hex(u32(t, 36), 8);
    con.print("\n");
    int cpus = 0;
    for (uint32_t o = 44; o + 2 <= u32(t, 4); o += u8(t, static_cast<int>(o) + 1)) {
        const uint8_t* e = t + o;
        if (e[1] == 0) {
            break;                              // a zero length would loop forever
        }
        if (e[0] == 0) {
            con.print("    type 0 processor local APIC: processor UID ");
            con.dec(e[2]);
            con.print(", APIC ID ");
            con.dec(e[3]);
            con.print(", flags ");
            con.hex(u32(e, 4), 8);
            con.print("\n");
            ++cpus;
        } else if (e[0] == 1) {
            con.print("    type 1 I/O APIC: ID ");
            con.dec(e[2]);
            con.print(", address ");
            con.hex(u32(e, 4), 8);
            con.print(", first GSI ");
            con.dec(u32(e, 8));
            con.print("\n");
        } else if (e[0] == 2) {
            con.print("    type 2 interrupt source override: ISA IRQ ");
            con.dec(e[3]);
            con.print(" -> GSI ");
            con.dec(u32(e, 4));
            con.print(", flags ");
            con.hex(u16(e, 8), 4);
            con.print("\n");
        } else {
            con.print("    type ");
            con.dec(e[0]);
            con.print(" (length ");
            con.dec(e[1]);
            con.print(")\n");
        }
    }
    con.print("    processors listed: ");
    con.dec(static_cast<uint64_t>(cpus));
    con.print("\n");
}

void acpi(Console& con, const uint8_t* rsdp)
{
    con.print("RSDP at ");
    con.hex(reinterpret_cast<uint64_t>(rsdp));
    con.print(": signature '");
    chars(con, rsdp, 8);
    con.print("', revision ");
    con.dec(u8(rsdp, 15));
    con.print(", OEM '");
    chars(con, rsdp + 9, 6);
    con.print("', checksum (first 20 bytes) ");
    con.print(sums_to_zero(rsdp, 20) ? "ok" : "BAD");
    con.print(", extended checksum (");
    con.dec(u32(rsdp, 20));
    con.print(" bytes) ");
    con.print(sums_to_zero(rsdp, u32(rsdp, 20)) ? "ok" : "BAD");
    con.print("\n");
    const uint8_t* xsdt = ptr(u64(rsdp, 24));
    con.print("XSDT at ");
    con.hex(u64(rsdp, 24));
    con.print(": ");
    header(con, xsdt);
    const uint32_t entries = (u32(xsdt, 4) - 36) / 8;
    bool fpdt = false;
    for (uint32_t i = 0; i < entries; ++i) {
        const uint64_t address = u64(xsdt, 36 + static_cast<int>(i) * 8);
        const uint8_t* t = ptr(address);
        con.print("  ");
        con.hex(address, 8);
        con.print("  ");
        header(con, t);
        const uint32_t sig = u32(t, 0);
        if (sig == u32(reinterpret_cast<const uint8_t*>("APIC"), 0)) {
            madt(con, t);
        } else if (sig == u32(reinterpret_cast<const uint8_t*>("HPET"), 0)) {
            con.print("    HPET registers at ");
            con.hex(u64(t, 44), 8);
            con.print("\n");
        } else if (sig == u32(reinterpret_cast<const uint8_t*>("MCFG"), 0)) {
            con.print("    PCI Express configuration space (ECAM) at ");
            con.hex(u64(t, 44), 8);
            con.print(", segment ");
            con.dec(u16(t, 52));
            con.print(", buses ");
            con.dec(u8(t, 54));
            con.print("-");
            con.dec(u8(t, 55));
            con.print("\n");
        } else if (sig == u32(reinterpret_cast<const uint8_t*>("FACP"), 0)) {
            const uint64_t dsdt = u32(t, 4) >= 148 && u64(t, 140) != 0 ? u64(t, 140) : u32(t, 40);
            con.print("    DSDT at ");
            con.hex(dsdt, 8);
            con.print(": ");
            header(con, ptr(dsdt));
        } else if (sig == u32(reinterpret_cast<const uint8_t*>("FPDT"), 0)) {
            fpdt = true;
        }
    }
    con.print(fpdt ? "FPDT present\n" : "FPDT (firmware performance data table): not in the XSDT\n");
}

const char* smbios_string(const uint8_t* s, uint8_t index)  // strings follow the formatted area
{
    if (index == 0) {
        return "(none)";
    }
    const char* p = reinterpret_cast<const char*>(s + s[1]);
    for (uint8_t i = 1; i < index && *p != '\0'; ++i) {
        while (*p != '\0') {
            ++p;
        }
        ++p;
    }
    return p;
}

void smbios3(Console& con, const uint8_t* ep)
{
    con.print("SMBIOS 3 entry point at ");
    con.hex(reinterpret_cast<uint64_t>(ep));
    con.print(": anchor '");
    chars(con, ep, 5);
    con.print("', version ");
    con.dec(ep[7]);
    con.print(".");
    con.dec(ep[8]);
    con.print(", checksum ");
    con.print(sums_to_zero(ep, ep[6]) ? "ok" : "BAD");
    con.print(", structures at ");
    con.hex(u64(ep, 16), 8);
    con.print(", at most ");
    con.dec(u32(ep, 12));
    con.print(" bytes\n");
    const uint8_t* s = ptr(u64(ep, 16));
    const uint8_t* end = s + u32(ep, 12);
    while (s + 4 <= end) {
        con.print("  type ");
        con.dec(s[0]);
        con.print(" handle ");
        con.hex(u16(s, 2), 4);
        if (s[0] == 0) {
            con.print("  firmware vendor '");
            con.print(smbios_string(s, s[4]));
            con.print("' version '");
            con.print(smbios_string(s, s[5]));
            con.print("'");
        } else if (s[0] == 1) {
            con.print("  system manufacturer '");
            con.print(smbios_string(s, s[4]));
            con.print("' product '");
            con.print(smbios_string(s, s[5]));
            con.print("'");
        } else if (s[0] == 4) {
            con.print("  processor socket '");
            con.print(smbios_string(s, s[4]));
            con.print("'");
        } else if (s[0] == 17) {
            con.print("  memory device '");
            con.print(smbios_string(s, s[0x10]));
            con.print("' size field ");
            con.hex(u16(s, 0x0c), 4);
        }
        con.print("\n");
        if (s[0] == 127) {
            break;                              // end-of-table structure
        }
        const uint8_t* p = s + s[1];            // skip the strings: they end with two zero bytes
        while (p + 1 < end && (p[0] != 0 || p[1] != 0)) {
            ++p;
        }
        s = p + 2;
    }
}

uint64_t rdtsc()
{
    uint32_t lo = 0, hi = 0;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return static_cast<uint64_t>(hi) << 32 | lo;
}

}  // namespace

extern "C" efi::Status efi_main(efi::Handle /*image*/, efi::SystemTable* st)
{
    const uint64_t tsc = rdtsc();
    Console con(st->con_out);
    con.print("F3-12 tables: time-stamp counter at entry ");
    con.dec(tsc);
    con.print("\nUEFI revision ");
    con.dec(st->hdr.revision >> 16);
    con.print(".");
    con.dec((st->hdr.revision & 0xffff) / 10);
    con.print(", firmware revision ");
    con.hex(st->firmware_revision, 8);
    con.print(", ");
    con.dec(st->number_of_table_entries);
    con.print(" configuration-table entries\n");
    const uint8_t* rsdp = nullptr;
    const uint8_t* sm3 = nullptr;
    for (uint64_t i = 0; i < st->number_of_table_entries; ++i) {
        const efi::ConfigurationTable& e = st->configuration_table[i];
        con.print("  ");
        con.hex(e.vendor_guid.data1, 8);
        con.print("-...  at ");
        con.hex(reinterpret_cast<uint64_t>(e.vendor_table));
        if (efi::same(e.vendor_guid, efi::kAcpi20TableGuid)) {
            con.print("  ACPI 2.0+ RSDP");
            rsdp = static_cast<const uint8_t*>(e.vendor_table);
        } else if (efi::same(e.vendor_guid, efi::kAcpi10TableGuid)) {
            con.print("  ACPI 1.0 RSDP");
        } else if (efi::same(e.vendor_guid, efi::kSmbios3TableGuid)) {
            con.print("  SMBIOS 3 entry point");
            sm3 = static_cast<const uint8_t*>(e.vendor_table);
        } else if (efi::same(e.vendor_guid, efi::kSmbiosTableGuid)) {
            con.print("  SMBIOS (32-bit) entry point");
        }
        con.print("\n");
    }
    if (rsdp != nullptr) {
        acpi(con, rsdp);
    }
    if (sm3 != nullptr) {
        smbios3(con, sm3);
    }
    con.print("F3-12 tables: done\n");
    qemu_exit(0x10);
    return efi::kSuccess;
}
