// edid_test.cc - DR405 F4-46: host unit tests of edid.h on EDID blobs recorded from QEMU.
// Each blob file is named edid_<W>x<H>.hex: the resolution QEMU was asked for. The test
// checks the header, the checksum and that the preferred timing has exactly that size,
// then prints the timing as Linux's struct drm_mode_modeinfo would hold it.
//   usage: edid_test <blob.hex>...
#include <cstdio>
#include <cstring>
#include <drm/drm_mode.h>
#include <fstream>
#include <string>
#include "edid.h"

namespace {
bool load(const char* path, std::array<uint8_t, 128>& b)
{
    std::ifstream in(path);
    unsigned v = 0;
    size_t n = 0;
    while (n < b.size() && (in >> std::hex >> v)) b[n++] = static_cast<uint8_t>(v);
    return n == b.size();
}

drm_mode_modeinfo to_drm(const edid::Timing& t)
{
    drm_mode_modeinfo m{};
    m.clock = t.pixel_clock_khz;
    m.hdisplay = t.hactive;
    m.hsync_start = static_cast<uint16_t>(t.hactive + t.hsync_offset);
    m.hsync_end = static_cast<uint16_t>(m.hsync_start + t.hsync_width);
    m.htotal = static_cast<uint16_t>(t.hactive + t.hblank);
    m.vdisplay = t.vactive;
    m.vsync_start = static_cast<uint16_t>(t.vactive + t.vsync_offset);
    m.vsync_end = static_cast<uint16_t>(m.vsync_start + t.vsync_width);
    m.vtotal = static_cast<uint16_t>(t.vactive + t.vblank);
    // refresh in Hz, rounded: pixels per second / pixels per frame
    m.vrefresh = static_cast<uint32_t>((uint64_t{m.clock} * 1000 + uint64_t{m.htotal} * m.vtotal / 2) /
                                       (uint64_t{m.htotal} * m.vtotal));
    std::snprintf(m.name, sizeof m.name, "%dx%d", m.hdisplay, m.vdisplay);
    return m;
}
}  // namespace

int main(int argc, char** argv)
{
    int failed = 0;
    for (int i = 1; i < argc; ++i) {
        std::array<uint8_t, 128> b{};
        int w = 0, h = 0;
        const char* base = std::strrchr(argv[i], '/') ? std::strrchr(argv[i], '/') + 1 : argv[i];
        std::sscanf(base, "edid_%dx%d", &w, &h);
        if (!load(argv[i], b)) {
            std::printf("%s: fewer than 128 bytes\n", base);
            ++failed;
            continue;
        }
        const edid::Info e = edid::parse(b);
        const edid::Timing& t = e.preferred;
        const bool size_ok = e.has_preferred && t.hactive == w && t.vactive == h;
        const bool ok = e.header_ok && e.checksum_ok && size_ok;
        std::printf("%s: header %s, checksum %s, manufacturer %s, product 0x%04x, year %d, EDID %d.%d, "
                    "extensions %d, name \"%s\"\n", base, e.header_ok ? "ok" : "BAD", e.checksum_ok ? "ok" : "BAD",
                    e.manufacturer.c_str(), e.product, e.year, e.version, e.revision, e.extensions, e.name.c_str());
        if (e.has_preferred) {
            const drm_mode_modeinfo m = to_drm(t);
            std::printf("  preferred: %u kHz  h %u %u %u %u  v %u %u %u %u  %s%s%s  -> \"%s\" %u Hz\n", m.clock,
                        m.hdisplay, m.hsync_start, m.hsync_end, m.htotal, m.vdisplay, m.vsync_start, m.vsync_end,
                        m.vtotal, t.hsync_positive ? "+hsync" : "-hsync", t.vsync_positive ? " +vsync" : " -vsync",
                        t.interlaced ? " interlaced" : "", m.name, m.vrefresh);
        }
        std::printf("  test (requested %dx%d): %s\n", w, h, ok ? "PASS" : "FAIL");
        if (!ok) ++failed;
    }
    std::printf("%d of %d blobs failed\n", failed, argc - 1);
    return failed == 0 && argc > 1 ? 0 : 1;
}
