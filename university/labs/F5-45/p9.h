// p9.h - DS403 kernel, F5-45: build and read 9P messages (9P2000 with the 9P2000.u fields
// that QEMU's server expects). All integers little-endian; strings are len[2] + bytes.
#pragma once
#include <stdint.h>

enum : uint8_t {
    Tversion = 100, Rversion, Tauth, Rauth, Tattach, Rattach, Terror, Rerror, Tflush, Rflush,
    Twalk, Rwalk, Topen, Ropen, Tcreate, Rcreate, Tread, Rread, Twrite, Rwrite, Tclunk, Rclunk,
    Tremove, Rremove, Tstat, Rstat, Twstat, Rwstat
};
constexpr uint16_t NOTAG = 0xFFFF;
constexpr uint32_t NOFID = 0xFFFFFFFFu;

struct P9Out {                                  // a message being built
    uint8_t buf[16384];
    uint32_t n;
    void begin(uint8_t type, uint16_t tag);
    void u8(uint8_t v);
    void u16(uint16_t v);
    void u32(uint32_t v);
    void u64(uint64_t v);
    void str(const char* s);
    void bytes(const void* p, uint32_t len);
    uint32_t finish();                          // writes size[4]; returns it
};

struct P9In {                                   // a message being read
    const uint8_t* p;
    uint32_t n, at;
    uint8_t type() const { return p[4]; }
    uint16_t tag() const { return static_cast<uint16_t>(p[5] | (p[6] << 8)); }
    void start() { at = 7; }
    uint8_t u8();
    uint16_t u16();
    uint32_t u32();
    uint64_t u64();
    int str(char* out, int cap);                // copies a string; returns its length
};

const char* p9_name(uint8_t type);
void p9_hex(const uint8_t* p, uint32_t n);      // dump bytes as hex, 16 per line
