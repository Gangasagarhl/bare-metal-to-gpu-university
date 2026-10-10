// romcheck.cpp - DR405 F4-44: parse the option ROM files QEMU gives its display devices
// (SeaBIOS's VGA BIOS images, as installed in the build container) with romparse.h. The
// lab kernel parses the same images read through each device's ROM BAR; the hashes must
// agree, which proves the kernel's read path and the parser's offsets on these images.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "romparse.h"

int main()
{
    const char* files[] = {"vgabios-stdvga.bin", "vgabios-bochs-display.bin", "vgabios-virtio.bin", "vgabios-ati.bin"};
    int bad = 0;
    for (const char* f : files) {
        const std::string path = std::string("/usr/share/seabios/") + f;
        std::ifstream in(path, std::ios::binary);
        const std::vector<unsigned char> b((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (b.empty()) {
            std::printf("%-26s missing\n", f);
            ++bad;
            continue;
        }
        const rom::Info r = rom::parse(b.data(), b.size());
        std::printf("%-26s file %5zu bytes | 55AA %s, PCIR at 0x%04x %s, vendor %04x device %04x class %06x, "
                    "image %5u bytes, code type %u, last %s, checksum %s, fnv1a(image) 0x%08x\n",
                    f, b.size(), r.header_ok ? "yes" : "NO", r.pcir_offset, r.pcir_ok ? "ok" : "BAD", r.vendor,
                    r.device, r.class_code, r.image_length, r.code_type, r.last_image ? "yes" : "no",
                    r.checksum_ok ? "ok" : "BAD", rom::fnv1a(b.data(), r.image_length));
        if (!r.header_ok || !r.pcir_ok || !r.checksum_ok) ++bad;
    }
    return bad == 0 ? 0 : 1;
}
