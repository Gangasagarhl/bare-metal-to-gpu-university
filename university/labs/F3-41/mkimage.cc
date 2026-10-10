// mkimage.cc - F3-41 Listing 8 (host tool): wrap a raw application binary into a slot image:
// a 256-byte header area (magic, version, size, load address, SHA-256) followed by the code.
//   mkimage <app.bin> <version> <load address, hex> <out.img>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

#include "image.h"
#include "sha256.h"

int main(int argc, char** argv)
{
    if (argc != 5) {
        std::fprintf(stderr, "usage: mkimage app.bin version loadaddr out.img\n");
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    const std::vector<uint8_t> payload((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    slots::Header h{};
    h.magic = slots::kMagic;
    h.version = static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10));
    h.payloadSize = static_cast<uint32_t>(payload.size());
    h.loadAddress = static_cast<uint32_t>(std::strtoul(argv[3], nullptr, 16));
    Sha256 sha;
    sha.update(payload.data(), payload.size());
    sha.finish(h.sha256);
    std::vector<uint8_t> out(slots::kHeaderSize, 0xFF);           // erased flash reads 0xFF
    const auto* hp = reinterpret_cast<const uint8_t*>(&h);
    std::copy(hp, hp + sizeof h, out.begin());
    out.insert(out.end(), payload.begin(), payload.end());
    std::ofstream(argv[4], std::ios::binary).write(reinterpret_cast<const char*>(out.data()),
                                                    static_cast<std::streamsize>(out.size()));
    std::printf("%s: version %u, %u payload bytes, load address 0x%08x, sha256 ", argv[4],
                h.version, h.payloadSize, h.loadAddress);
    for (uint8_t b : h.sha256) { std::printf("%02x", b); }
    std::printf("\n");
    return 0;
}
