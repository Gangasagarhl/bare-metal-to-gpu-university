// F3-49 ntfs.h: the NTFS on-disk pieces shared by ntfsgen (makes test images) and ntfsread.
// Every offset here was written from memory of the Linux-NTFS documentation and Carrier's
// "File System Forensic Analysis"; only what libblkid checks has been confirmed (see the chapter).
#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

using Bytes = std::vector<uint8_t>;
inline uint16_t le16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }
inline uint32_t le32(const uint8_t* p) { return le16(p) | uint32_t(le16(p + 2)) << 16; }
inline uint64_t le64(const uint8_t* p) { return le32(p) | uint64_t(le32(p + 4)) << 32; }
inline void put16(uint8_t* p, uint16_t v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); }
inline void put32(uint8_t* p, uint32_t v) { put16(p, uint16_t(v)); put16(p + 2, uint16_t(v >> 16)); }
inline void put64(uint8_t* p, uint64_t v) { put32(p, uint32_t(v)); put32(p + 4, uint32_t(v >> 32)); }

// attribute types
constexpr uint32_t kStdInfo = 0x10, kFileName = 0x30, kVolName = 0x60, kVolInfo = 0x70,
                   kData = 0x80, kIndexRoot = 0x90, kIndexAlloc = 0xA0, kBitmap = 0xB0,
                   kEnd = 0xFFFFFFFF;
constexpr unsigned kSector = 512;          // the fixup stride is 512 bytes whatever the sector size
constexpr uint64_t kRefMask = 0x0000FFFFFFFFFFFFull;  // file reference = record | sequence << 48

// Update sequence ("fixups"). Before a multi-sector record is written, the last two bytes of
// every 512-byte stride are saved in the update sequence array and replaced by the update
// sequence number. A reader checks that every stride still ends in that number (else the write
// was torn: some sectors are new, some old) and puts the saved bytes back.
inline void protect(uint8_t* rec, size_t size, uint16_t usn) {
    const uint16_t ofs = le16(rec + 4), count = le16(rec + 6);
    if (count != size / kSector + 1) throw std::runtime_error("update sequence count");
    put16(rec + ofs, usn);
    for (unsigned i = 1; i < count; i++) {
        uint8_t* tail = rec + i * kSector - 2;
        std::memcpy(rec + ofs + 2 * i, tail, 2);
        put16(tail, usn);
    }
}
// Returns 0 when every stride matched, else the number (1-based) of the first stride that
// did not; nothing is trusted then.
inline unsigned unprotect(uint8_t* rec, size_t size) {
    const uint16_t ofs = le16(rec + 4), count = le16(rec + 6);
    if (count != size / kSector + 1 || ofs + 2u * count > kSector) return 1;
    const uint16_t usn = le16(rec + ofs);
    for (unsigned i = 1; i < count; i++)
        if (le16(rec + i * kSector - 2) != usn) return i;
#ifndef FORENSIC_SKIP_RESTORE
    for (unsigned i = 1; i < count; i++) std::memcpy(rec + i * kSector - 2, rec + ofs + 2 * i, 2);
#endif
    return 0;
}

// Run lists ("mapping pairs"): a header byte whose low nibble is the size of the length field
// and high nibble the size of the offset field, then both little-endian. The offset is signed
// and relative to the previous run's LCN; an offset size of 0 means a sparse run; a 0 byte ends.
struct Run { int64_t lcn; uint64_t len; };     // lcn -1: sparse
inline std::vector<Run> decodeRuns(const uint8_t* p, const uint8_t* end) {
    std::vector<Run> runs;
    int64_t lcn = 0;
    while (p < end && *p) {
        const unsigned nl = *p & 15, no = *p >> 4;
        if (nl == 0 || nl > 8 || no > 8 || p + 1 + nl + no > end) throw std::runtime_error("bad run list");
        uint64_t len = 0;
        for (unsigned i = 0; i < nl; i++) len |= uint64_t(p[1 + i]) << 8 * i;
        int64_t delta = 0;
        for (unsigned i = 0; i < no; i++) delta |= int64_t(p[1 + nl + i]) << 8 * i;
        if (no && (p[nl + no] & 0x80)) delta -= int64_t(1) << 8 * no;   // sign-extend
        if (no) { lcn += delta; runs.push_back({lcn, len}); } else runs.push_back({-1, len});
        p += 1 + nl + no;
    }
    return runs;
}
inline unsigned bytesFor(int64_t v, bool isSigned) {
    unsigned n = 1;
    while (isSigned ? (v < -(int64_t(1) << (8 * n - 1)) || v >= (int64_t(1) << (8 * n - 1)))
                    : (uint64_t(v) >> (8 * n)) != 0)
        n++;
    return n;
}
inline Bytes encodeRuns(const std::vector<Run>& runs) {
    Bytes out;
    int64_t prev = 0;
    for (const Run& r : runs) {
        const unsigned nl = bytesFor(int64_t(r.len), false);
        const int64_t delta = r.lcn < 0 ? 0 : r.lcn - prev;
        const unsigned no = r.lcn < 0 ? 0 : bytesFor(delta, true);
        out.push_back(uint8_t(no << 4 | nl));
        for (unsigned i = 0; i < nl; i++) out.push_back(uint8_t(r.len >> 8 * i));
        for (unsigned i = 0; i < no; i++) out.push_back(uint8_t(uint64_t(delta) >> 8 * i));
        if (r.lcn >= 0) prev = r.lcn;
    }
    out.push_back(0);
    return out;
}

// Names are UTF-16LE on disk. Both tools handle the Basic Multilingual Plane only.
inline std::u16string fromUtf8(const std::string& s) {
    std::u16string out;
    for (size_t i = 0; i < s.size();) {
        const uint8_t c = uint8_t(s[i]);
        if (c < 0x80) { out += char16_t(c); i += 1; }
        else if ((c & 0xE0) == 0xC0) { out += char16_t((c & 31) << 6 | (s[i + 1] & 63)); i += 2; }
        else { out += char16_t((c & 15) << 12 | (s[i + 1] & 63) << 6 | (s[i + 2] & 63)); i += 3; }
    }
    return out;
}
inline std::string toUtf8(const std::u16string& s) {
    std::string out;
    for (char16_t c : s) {
        if (c < 0x80) out += char(c);
        else if (c < 0x800) { out += char(0xC0 | c >> 6); out += char(0x80 | (c & 63)); }
        else { out += char(0xE0 | c >> 12); out += char(0x80 | (c >> 6 & 63)); out += char(0x80 | (c & 63)); }
    }
    return out;
}
// The upper-case table both tools use (the image's $UpCase holds the same mapping): ASCII,
// Latin-1 and the Greek small letters. Real Windows tables cover much more of Unicode.
inline char16_t upcase(char16_t c) {
    if (c >= u'a' && c <= u'z') return char16_t(c - 32);
    if (c >= 0xE0 && c <= 0xFE && c != 0xF7) return char16_t(c - 32);
    if (c >= 0x3B1 && c <= 0x3C9 && c != 0x3C2) return char16_t(c - 32);
    return c;
}
// Directory index order ("collation rule 1, file name"): compare upper-cased UTF-16 units.
inline int collate(const std::u16string& a, const std::u16string& b) {
    for (size_t i = 0; i < a.size() && i < b.size(); i++) {
        const char16_t x = upcase(a[i]), y = upcase(b[i]);
        if (x != y) return x < y ? -1 : 1;
    }
    return a.size() == b.size() ? 0 : a.size() < b.size() ? -1 : 1;
}
