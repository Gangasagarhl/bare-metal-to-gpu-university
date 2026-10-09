// smbios4.h - read the firmware's SMBIOS tables (read only): the machine's own name for itself.
// The entry-point and structure layouts here were written from memory of the DMTF SMBIOS
// Reference Specification (title only in this build); every table found is checksum-tested,
// and the lab cross-checks the strings against values given to QEMU with -smbios.
#pragma once
#include <stdint.h>

namespace smbios4 {

struct Entry { uintptr_t at; int major, minor; uintptr_t table; uint32_t table_len; bool ok; bool v3; };

bool find(Entry& e);                                   // scan 0xF0000..0xFFFFF on 16-byte steps
// Calls visit(type, handle, formatted area, its length, strings) for every structure.
using Visitor = void (*)(uint8_t type, uint16_t handle, const uint8_t* s, uint8_t len,
                         const char* strings);
int walk(const Entry& e, Visitor visit);
const char* string_at(const char* strings, uint8_t index);   // index 1..n; 0 or missing: ""

} // namespace smbios4
