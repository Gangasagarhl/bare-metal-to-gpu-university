// image.h - F3-41 Listing 5: the slot layout and image header shared by the on-target boot
// loader (boot.cc), the application images (app.cc) and the host image tool (mkimage.cc).
// This is the university's own teaching format, not MCUboot's (see the chapter).
#pragma once
#include <stdint.h>

namespace slots {

inline constexpr uint32_t kSlotA = 0x00004000;     // 32 KiB each, in the code memory
inline constexpr uint32_t kSlotB = 0x0000C000;
inline constexpr uint32_t kSlotSize = 0x8000;
inline constexpr uint32_t kHeaderSize = 0x100;     // the application starts 256 bytes in
inline constexpr uint32_t kMagic = 0x3530334F;     // "O305" in little-endian bytes

struct Header {
    uint32_t magic;
    uint32_t version;
    uint32_t payloadSize;      // bytes after the 256-byte header area
    uint32_t loadAddress;      // where the payload was linked to run (slot + 0x100)
    uint8_t sha256[32];        // SHA-256 of the payload
};
static_assert(sizeof(Header) == 48, "header layout");

}  // namespace slots
