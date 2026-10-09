// acpi.h - DR301: just enough ACPI table reading for the drivers of this course:
// find the RSDP, walk the RSDT or XSDT, and hand back a table by its signature.
#pragma once
#include <stdint.h>

struct AcpiHeader {              // the common header of every ACPI system description table
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));
static_assert(sizeof(AcpiHeader) == 36, "ACPI header is 36 bytes");

namespace acpi {
bool init();                                  // false: no valid RSDP found
const AcpiHeader* find(const char sig[4]);    // nullptr if absent or bad checksum
bool ecam(uint64_t& base, uint8_t& bus_start, uint8_t& bus_end);   // from the MCFG table
}
