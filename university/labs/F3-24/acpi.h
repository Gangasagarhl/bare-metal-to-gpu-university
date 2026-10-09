// acpi.h - F3-24: finding and checking the ACPI tables (RSDP, RSDT or XSDT, then each table).
// Layouts are written from memory of the ACPI Specification ("ACPI Software Programming
// Model"); the chapter's unverified box lists what must be checked there.
#pragma once
#include <cstdint>

namespace acpi {

struct [[gnu::packed]] Header {     // the 36-byte header every system description table starts with
    char signature[4];
    uint32_t length;                // whole table, header included
    uint8_t revision;
    uint8_t checksum;               // all 'length' bytes must add up to 0 (mod 256)
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};
static_assert(sizeof(Header) == 36);

// All 'length' bytes must add up to 0 modulo 256. Pure logic: the host test uses it too.
inline bool checksum_ok(const void* p, uint32_t length)
{
    const auto* b = static_cast<const uint8_t*>(p);
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; ++i) {
        sum = static_cast<uint8_t>(sum + b[i]);
    }
    return sum == 0;
}

bool init();                                         // finds the RSDP, lists and checks every table
const Header* find(const char sig[4]);               // nullptr if absent or rejected
uint64_t rsdp_phys();

} // namespace acpi
