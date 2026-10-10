// p9.cc - DS403 kernel, F5-45: 9P message encoding helpers.
#include "p9.h"
#include "k.h"

void P9Out::begin(uint8_t type, uint16_t tag) { n = 4; u8(type); u16(tag); }
void P9Out::u8(uint8_t v) { buf[n++] = v; }
void P9Out::u16(uint16_t v) { u8(static_cast<uint8_t>(v)); u8(static_cast<uint8_t>(v >> 8)); }
void P9Out::u32(uint32_t v) { u16(static_cast<uint16_t>(v)); u16(static_cast<uint16_t>(v >> 16)); }
void P9Out::u64(uint64_t v) { u32(static_cast<uint32_t>(v)); u32(static_cast<uint32_t>(v >> 32)); }
void P9Out::str(const char* s)
{
    uint16_t len = static_cast<uint16_t>(kstrlen(s));
    u16(len);
    bytes(s, len);
}
void P9Out::bytes(const void* p, uint32_t len) { memcpy(buf + n, p, len); n += len; }
uint32_t P9Out::finish()
{
    buf[0] = static_cast<uint8_t>(n);
    buf[1] = static_cast<uint8_t>(n >> 8);
    buf[2] = static_cast<uint8_t>(n >> 16);
    buf[3] = static_cast<uint8_t>(n >> 24);
    return n;
}

uint8_t P9In::u8() { return at < n ? p[at++] : 0; }
uint16_t P9In::u16() { uint16_t lo = u8(); return static_cast<uint16_t>(lo | (u8() << 8)); }
uint32_t P9In::u32() { uint32_t lo = u16(); return lo | (static_cast<uint32_t>(u16()) << 16); }
uint64_t P9In::u64() { uint64_t lo = u32(); return lo | (static_cast<uint64_t>(u32()) << 32); }
int P9In::str(char* out, int cap)
{
    uint16_t len = u16();
    int i = 0;
    for (; i < len; ++i) {
        char c = static_cast<char>(u8());
        if (i + 1 < cap) out[i] = c;
    }
    out[i + 1 < cap ? i : cap - 1] = '\0';
    return len;
}

const char* p9_name(uint8_t t)
{
    static const char* names[] = {"Tversion", "Rversion", "Tauth", "Rauth", "Tattach", "Rattach",
        "Terror", "Rerror", "Tflush", "Rflush", "Twalk", "Rwalk", "Topen", "Ropen", "Tcreate",
        "Rcreate", "Tread", "Rread", "Twrite", "Rwrite", "Tclunk", "Rclunk", "Tremove", "Rremove",
        "Tstat", "Rstat", "Twstat", "Rwstat"};
    return (t >= 100 && t <= 127) ? names[t - 100] : "unknown";
}

void p9_hex(const uint8_t* p, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) {
        if (i % 16 == 0) kprintf("   ");
        kprintf(" %02x", p[i]);
        if (i % 16 == 15 || i + 1 == n) kprintf("\n");
    }
}
