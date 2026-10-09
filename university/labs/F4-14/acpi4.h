// acpi4.h - find the ACPI tables the firmware left in memory (read only).
#pragma once
#include <stdint.h>

namespace acpi4 {

struct Table { uintptr_t addr; uint32_t length; char sig[5]; bool checksum_ok; };

bool init();                                   // finds the RSDP and the RSDT; false if none
int count();                                   // number of tables listed in the RSDT
Table at(int i);                               // i-th table of the RSDT
bool find(const char* sig, Table& t);          // a table by signature ("FACP", "APIC", ...)
bool dsdt(Table& t);                           // the DSDT, through the FADT's pointer
void dump_hex(const Table& t, const char* tag);   // "tag offset: 32 bytes in hex" lines

} // namespace acpi4
