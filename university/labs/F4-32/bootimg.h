// bootimg.h - F4-32: this course's boot image format and the checks every stage applies.
// Real boot ROMs each have their own vendor format and most verify a signature with a key
// fused into the SoC; this one only has a CRC-32, which detects damage, not tampering.
#pragma once
#include <stdint.h>

struct BootImage {           // 32 bytes, little-endian, followed by `size` bytes of payload
    char magic[4];           // "DR4I"
    uint32_t load;           // where to copy the payload (physical address)
    uint32_t size;           // payload bytes
    uint32_t crc32;          // CRC-32 (the zlib / IEEE 802.3 polynomial) of the payload
    char name[16];           // "BL2", "BL31", "BL33" ...
};

inline uint32_t crc32(const uint8_t* p, uint32_t n)
{
    uint32_t c = 0xffffffffu;
    for (uint32_t i = 0; i < n; ++i) {
        c ^= p[i];
        for (int b = 0; b < 8; ++b) {
            c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
        }
    }
    return ~c;
}

// Returns nullptr if the image is good, else a short reason. On success the payload has been
// copied to img->load.
inline const char* load_image(const BootImage* img)
{
    if (img->magic[0] != 'D' || img->magic[1] != 'R' || img->magic[2] != '4' || img->magic[3] != 'I') {
        return "no image (bad magic)";
    }
    const auto* src = reinterpret_cast<const uint8_t*>(img + 1);
    if (crc32(src, img->size) != img->crc32) {
        return "CRC-32 mismatch (damaged image)";
    }
    auto* dst = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(img->load));
    for (uint32_t i = 0; i < img->size; ++i) {
        dst[i] = src[i];
    }
    return nullptr;
}
