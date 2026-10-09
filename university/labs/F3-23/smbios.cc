// smbios.cc - F3-23: SMBIOS entry-point search and structure-table walk.
// Anchor strings, entry-point offsets and structure field offsets are written from memory
// and must be checked in the DMTF "System Management BIOS (SMBIOS) Reference Specification"
// (see the chapter's unverified box); the lab shows what QEMU 8.2.2 with SeaBIOS provides.
#include "smbios.h"
#include <cstdint>
#include "kprint.h"
#include "log.h"
#include "pmm.h"

namespace smbios {
namespace {

const uint8_t* phys(uint64_t a)
{
    return static_cast<const uint8_t*>(pmm::phys_to_virt(a));
}

bool sum_is_zero(const uint8_t* p, unsigned n)
{
    uint8_t s = 0;
    for (unsigned i = 0; i < n; ++i) {
        s = static_cast<uint8_t>(s + p[i]);
    }
    return s == 0;
}

// String number 'n' (1-based) of the structure at 's'; strings follow the formatted area.
const char* string_of(const uint8_t* s, uint8_t n)
{
    if (n == 0) {
        return "(none)";
    }
    const char* p = reinterpret_cast<const char*>(s + s[1]);   // s[1] = formatted-area length
    for (uint8_t i = 1; i < n && *p != '\0'; ++i) {
        while (*p != '\0') {
            ++p;
        }
        ++p;
    }
    return *p != '\0' ? p : "(missing)";
}

} // namespace

void print_banner()
{
    uint64_t table = 0, length = 0;
    for (uint64_t a = 0xF0000; a < 0x100000; a += 16) {
        const uint8_t* e = phys(a);
        if (e[0] == '_' && e[1] == 'S' && e[2] == 'M' && e[3] == '3' && e[4] == '_' && sum_is_zero(e, e[6])) {
            table = *reinterpret_cast<const uint64_t*>(e + 0x10);
            length = *reinterpret_cast<const uint32_t*>(e + 0x0C);
            klog(Level::Info, "smbios: 64-bit entry point at %05lx, version %u.%u, table %08lx, max %lu bytes",
                 a, unsigned{e[7]}, unsigned{e[8]}, table, length);
            break;
        }
        if (e[0] == '_' && e[1] == 'S' && e[2] == 'M' && e[3] == '_' && sum_is_zero(e, e[5])) {
            table = *reinterpret_cast<const uint32_t*>(e + 0x18);
            length = *reinterpret_cast<const uint16_t*>(e + 0x16);
            klog(Level::Info, "smbios: 32-bit entry point at %05lx, version %u.%u, table %08lx, %lu bytes",
                 a, unsigned{e[6]}, unsigned{e[7]}, table, length);
            break;
        }
    }
    if (table == 0) {
        klog(Level::Warn, "smbios: no entry point found");
        return;
    }
    const char* bios_vendor = "?";
    const char* bios_version = "?";
    const char* bios_date = "?";
    const char* sys_vendor = "?";
    const char* sys_product = "?";
    const char* board = "(no type 2 structure)";
    const uint8_t* s = phys(table);
    const uint8_t* end = s + length;
    int count = 0;
    while (s + 4 <= end && s[0] != 127) {          // type 127 marks the end of the table
        if (s[0] == 0) {
            bios_vendor = string_of(s, s[4]);
            bios_version = string_of(s, s[5]);
            bios_date = string_of(s, s[8]);
        } else if (s[0] == 1) {
            sys_vendor = string_of(s, s[4]);
            sys_product = string_of(s, s[5]);
        } else if (s[0] == 2) {
            board = string_of(s, s[5]);
        }
        ++count;
        const uint8_t* p = s + s[1];               // skip the formatted area, then the strings,
        while (!(p[0] == 0 && p[1] == 0)) {         // which end with two zero bytes
            ++p;
        }
        s = p + 2;
    }
    klog(Level::Info, "smbios: %d structures before the end marker", count);
    klog(Level::Info, "Booted on: %s %s, board %s", sys_vendor, sys_product, board);
    klog(Level::Info, "Firmware:  %s, version %s, date %s", bios_vendor, bios_version, bios_date);
}

} // namespace smbios
