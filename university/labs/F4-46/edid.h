// edid.h - DR405 F4-46: a small parser for the 128-byte EDID base block. The byte layout
// is written from the author's memory of the VESA E-EDID standard (not opened in this
// build); edid_test.cc checks it against EDID blobs recorded from QEMU (whose requested
// resolutions are known) and converts the preferred timing into Linux's struct
// drm_mode_modeinfo from <drm/drm_mode.h>.
#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace edid {
struct Timing {                    // one detailed timing descriptor
    uint32_t pixel_clock_khz = 0;
    uint16_t hactive = 0, hblank = 0, hsync_offset = 0, hsync_width = 0;
    uint16_t vactive = 0, vblank = 0, vsync_offset = 0, vsync_width = 0;
    bool interlaced = false, hsync_positive = false, vsync_positive = false;
};
struct Info {
    bool header_ok = false, checksum_ok = false;
    std::string manufacturer, name;
    uint16_t product = 0;
    int year = 0, version = 0, revision = 0, extensions = 0;
    bool has_preferred = false;
    Timing preferred;
};

inline Timing parse_dtd(const uint8_t* d)
{
    Timing t;
    t.pixel_clock_khz = uint32_t(d[0] | (d[1] << 8)) * 10;      // stored in units of 10 kHz
    t.hactive = static_cast<uint16_t>(d[2] | ((d[4] & 0xF0) << 4));
    t.hblank = static_cast<uint16_t>(d[3] | ((d[4] & 0x0F) << 8));
    t.vactive = static_cast<uint16_t>(d[5] | ((d[7] & 0xF0) << 4));
    t.vblank = static_cast<uint16_t>(d[6] | ((d[7] & 0x0F) << 8));
    t.hsync_offset = static_cast<uint16_t>(d[8] | ((d[11] & 0xC0) << 2));
    t.hsync_width = static_cast<uint16_t>(d[9] | ((d[11] & 0x30) << 4));
    t.vsync_offset = static_cast<uint16_t>((d[10] >> 4) | ((d[11] & 0x0C) << 2));
    t.vsync_width = static_cast<uint16_t>((d[10] & 0x0F) | ((d[11] & 0x03) << 4));
    t.interlaced = d[17] & 0x80;
    t.vsync_positive = d[17] & 0x04;
    t.hsync_positive = d[17] & 0x02;
    return t;
}

inline Info parse(const std::array<uint8_t, 128>& b)
{
    static const uint8_t hdr[8] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    Info r;
    r.header_ok = std::equal(hdr, hdr + 8, b.begin());
    uint8_t sum = 0;
    for (uint8_t x : b) sum = static_cast<uint8_t>(sum + x);
    r.checksum_ok = sum == 0;
    const uint16_t m = static_cast<uint16_t>((b[8] << 8) | b[9]);  // three 5-bit letters, 'A' = 1
    for (int shift : {10, 5, 0}) r.manufacturer += static_cast<char>('A' - 1 + ((m >> shift) & 0x1F));
    r.product = static_cast<uint16_t>(b[10] | (b[11] << 8));
    r.year = 1990 + b[17];
    r.version = b[18];
    r.revision = b[19];
    r.extensions = b[126];
    for (int k = 0; k < 4; ++k) {                                   // four 18-byte descriptors
        const uint8_t* d = &b[54 + 18 * k];
        if (d[0] || d[1]) {                                          // non-zero clock: a timing
            if (k == 0) {
                r.has_preferred = true;
                r.preferred = parse_dtd(d);
            }
        } else if (d[3] == 0xFC) {                                   // display product name
            for (int i = 5; i < 18 && d[i] != 0x0A; ++i) r.name += static_cast<char>(d[i]);
        }
    }
    return r;
}
}  // namespace edid
