// smbios4.cc - see smbios4.h. Offsets used (from memory, so checked by checksum and by the
// -smbios cross-check of the lab):
//   2.x entry "_SM_": [5] length, [6] major, [7] minor, [0x16] u16 table length,
//                     [0x18] u32 table address; checksum = all [length] bytes sum to 0
//   3.x entry "_SM3_": [6] length, [7] major, [8] minor, [0x0C] u32 maximum table size,
//                     [0x10] u64 table address; checksum as above
//   structure: [0] type, [1] length of the formatted area, [2..3] handle; then the strings,
//              each NUL-terminated, the set ending with a second NUL; type 127 = end of table.
#include "smbios4.h"

namespace smbios4 {

static uint8_t rd8(uintptr_t a) { return *reinterpret_cast<const volatile uint8_t*>(a); }
static uint16_t rd16(uintptr_t a) { return static_cast<uint16_t>(rd8(a) | (rd8(a + 1) << 8)); }
static uint32_t rd32(uintptr_t a) { return rd16(a) | (static_cast<uint32_t>(rd16(a + 2)) << 16); }

// The compiler must not see a constant low address (GCC treats it as a null-pointer offset).
static uintptr_t opaque(uintptr_t a) { asm volatile("" : "+r"(a)); return a; }

static bool sum_ok(uintptr_t a, unsigned len)
{
    uint8_t s = 0;
    for (unsigned i = 0; i < len; ++i) s = static_cast<uint8_t>(s + rd8(a + i));
    return s == 0;
}

bool find(Entry& e)
{
    for (uintptr_t a = opaque(0xF0000); a < 0x100000; a += 16) {
        if (rd8(a) == '_' && rd8(a + 1) == 'S' && rd8(a + 2) == 'M' && rd8(a + 3) == '3' &&
            rd8(a + 4) == '_') {
            e = {a, rd8(a + 7), rd8(a + 8), static_cast<uintptr_t>(rd32(a + 0x10)), rd32(a + 0x0C),
                 sum_ok(a, rd8(a + 6)), true};
            return true;
        }
        if (rd8(a) == '_' && rd8(a + 1) == 'S' && rd8(a + 2) == 'M' && rd8(a + 3) == '_') {
            e = {a, rd8(a + 6), rd8(a + 7), static_cast<uintptr_t>(rd32(a + 0x18)), rd16(a + 0x16),
                 sum_ok(a, rd8(a + 5)), false};
            return true;
        }
    }
    return false;
}

const char* string_at(const char* strings, uint8_t index)
{
    if (index == 0) return "";
    const char* p = strings;
    for (uint8_t i = 1; i < index; ++i) {
        if (*p == 0) return "";              // fewer strings than the index asks for
        while (*p) ++p;
        ++p;
    }
    return p;
}

int walk(const Entry& e, Visitor visit)
{
    uintptr_t p = e.table;
    const uintptr_t end = e.table + e.table_len;
    int n = 0;
    while (p + 4 <= end) {
        const uint8_t type = rd8(p), len = rd8(p + 1);
        if (len < 4) break;                                  // malformed: stop, do not loop
        const char* strings = reinterpret_cast<const char*>(p + len);
        visit(type, rd16(p + 2), reinterpret_cast<const uint8_t*>(p), len, strings);
        ++n;
        if (type == 127) break;
        uintptr_t q = p + len;                               // skip the string set
        while (q + 1 < end && !(rd8(q) == 0 && rd8(q + 1) == 0)) ++q;
        p = q + 2;
    }
    return n;
}

} // namespace smbios4
