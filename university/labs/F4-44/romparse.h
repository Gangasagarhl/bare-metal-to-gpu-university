// romparse.h - DR405 F4-44: parse a PCI expansion ROM image (header, PCI data structure,
// checksum) without writing anything. Pure code with no library, shared by the lab kernel
// and the host check (romcheck.cpp). Offsets are written from the author's memory of the
// PCI Firmware Specification, "PCI Expansion ROMs" (not opened in this build); the lab
// cross-checks them against each device's own configuration-space IDs.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace rom {
struct Info {
    bool header_ok = false, pcir_ok = false, checksum_ok = false, last_image = false;
    uint32_t init_size = 0;        // bytes, from header byte 2 (units of 512 bytes)
    uint16_t pcir_offset = 0;
    uint16_t vendor = 0, device = 0;
    uint32_t class_code = 0;       // class << 16 | subclass << 8 | prog-if
    uint32_t image_length = 0;     // bytes, from the PCI data structure (units of 512 bytes)
    uint8_t code_type = 0xFF;      // 0 = x86 legacy code
    uint8_t sum = 0;               // byte sum over image_length (0 means a valid checksum)
};

inline uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }

// 'img' points at a copy of (or a mapping of) the ROM; 'avail' is how many bytes may be read.
inline Info parse(const volatile uint8_t* img, size_t avail)
{
    Info r;
    if (avail < 0x1A || img[0] != 0x55 || img[1] != 0xAA) return r;
    r.header_ok = true;
    r.init_size = uint32_t{img[2]} * 512;
    r.pcir_offset = static_cast<uint16_t>(img[0x18] | (img[0x19] << 8));
    const size_t p = r.pcir_offset;
    if (p + 0x18 > avail || img[p] != 'P' || img[p + 1] != 'C' || img[p + 2] != 'I' || img[p + 3] != 'R') return r;
    r.pcir_ok = true;
    r.vendor = static_cast<uint16_t>(img[p + 4] | (img[p + 5] << 8));
    r.device = static_cast<uint16_t>(img[p + 6] | (img[p + 7] << 8));
    r.class_code = (uint32_t{img[p + 0xF]} << 16) | (uint32_t{img[p + 0xE]} << 8) | img[p + 0xD];
    r.image_length = uint32_t(img[p + 0x10] | (img[p + 0x11] << 8)) * 512;
    r.code_type = img[p + 0x14];
    r.last_image = img[p + 0x15] & 0x80;
    if (r.image_length == 0 || r.image_length > avail) return r;
    uint8_t s = 0;
    for (uint32_t i = 0; i < r.image_length; ++i) s = static_cast<uint8_t>(s + img[i]);
    r.sum = s;
    r.checksum_ok = s == 0;
    return r;
}

inline uint32_t fnv1a(const volatile uint8_t* p, size_t n)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; ++i) h = (h ^ p[i]) * 16777619u;
    return h;
}
}  // namespace rom
