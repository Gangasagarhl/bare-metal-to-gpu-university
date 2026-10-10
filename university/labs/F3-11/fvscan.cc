// fvscan.cc - list what is inside a UEFI PI firmware image (milestone A4, reading part).
// Finds firmware volumes (FV), walks the firmware files (FFS) in each, prints each file's type
// and the module name from its user-interface section, and opens LZMA-compressed sections with
// liblzma so that the volumes packed inside them can be walked too.
// The structure layouts were written from memory of the UEFI PI Specification, Volume 3
// ("Firmware Storage"); they are NOT verified against the text (see the chapter's unverified box).
// The run on the real OVMF image shows module names that only appear if the walk is right.
#include <lzma.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace {

using Bytes = std::vector<std::uint8_t>;

std::uint32_t u16(const Bytes& b, std::size_t o) { return b[o] | b[o + 1] << 8; }
std::uint32_t u24(const Bytes& b, std::size_t o) { return b[o] | b[o + 1] << 8 | b[o + 2] << 16; }
std::uint32_t u32(const Bytes& b, std::size_t o) { return u24(b, o) | static_cast<std::uint32_t>(b[o + 3]) << 24; }
std::uint64_t u64(const Bytes& b, std::size_t o) { return u32(b, o) | static_cast<std::uint64_t>(u32(b, o + 4)) << 32; }

std::string guid(const Bytes& b, std::size_t o)  // printed in the usual 8-4-4-4-12 form
{
    char s[40];
    std::snprintf(s, sizeof s, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", u32(b, o), u16(b, o + 4),
                  u16(b, o + 6), b[o + 8], b[o + 9], b[o + 10], b[o + 11], b[o + 12], b[o + 13],
                  b[o + 14], b[o + 15]);
    return s;
}

const char* file_type(unsigned t)
{
    switch (t) {
    case 0x01: return "RAW";
    case 0x02: return "FREEFORM";
    case 0x03: return "SECURITY_CORE";
    case 0x04: return "PEI_CORE";
    case 0x05: return "DXE_CORE";
    case 0x06: return "PEIM";
    case 0x07: return "DRIVER";
    case 0x09: return "APPLICATION";
    case 0x0a: return "MM";
    case 0x0b: return "FIRMWARE_VOLUME_IMAGE";
    case 0x0d: return "MM_CORE";
    case 0xf0: return "PAD";
    default:   return "OTHER";
    }
}

const std::string kLzmaGuid = "ee4e5898-3914-4259-9d6e-dc7bd79403cf";

void scan(const Bytes& data, const std::string& where, int depth);

Bytes lzma_unpack(const Bytes& in, std::size_t off, std::size_t len)
{
    // the "LZMA alone" format: 5 property bytes, then the unpacked size as a 64-bit number
    Bytes out(u64(in, off + 5));
    lzma_stream strm = LZMA_STREAM_INIT;
    if (lzma_alone_decoder(&strm, UINT64_MAX) != LZMA_OK) {
        return {};
    }
    strm.next_in = in.data() + off;
    strm.avail_in = len;
    strm.next_out = out.data();
    strm.avail_out = out.size();
    const lzma_ret r = lzma_code(&strm, LZMA_FINISH);
    lzma_end(&strm);
    if (r != LZMA_STREAM_END && r != LZMA_OK) {
        return {};
    }
    return out;
}

// Walks the sections of one file. With depth < 0 it only collects the module name (from the
// user-interface section); otherwise it opens compressed and volume sections and scans them.
std::string sections(const Bytes& fv, std::size_t start, std::size_t end, int depth)
{
    std::string name;
    std::size_t s = start;
    while (s + 4 <= end) {
        std::size_t size = u24(fv, s);
        std::size_t hdr = 4;
        const unsigned type = fv[s + 3];
        if (size == 0xffffff) {  // large section: the real size follows the header
            size = u32(fv, s + 4);
            hdr = 8;
        }
        if (size < hdr || s + size > end) {
            break;
        }
        if (type == 0x15) {      // user interface: the module's name in UCS-2
            for (std::size_t i = s + hdr; i + 1 < s + size && fv[i] != 0; i += 2) {
                name += static_cast<char>(fv[i]);
            }
        } else if (depth < 0) {
            // name-only pass: do not open anything
        } else if (type == 0x02 && guid(fv, s + hdr) == kLzmaGuid) {  // GUID-defined: LZMA
            const std::size_t data = s + u16(fv, s + hdr + 16);
            const Bytes inner = lzma_unpack(fv, data, s + size - data);
            std::printf("%*s  LZMA-compressed section: %zu bytes unpack to %zu bytes\n", depth * 4, "",
                        s + size - data, inner.size());
            scan(inner, "inside the LZMA section", depth + 1);
        } else if (type == 0x17) {  // a whole firmware volume stored as a section
            scan(Bytes(fv.begin() + static_cast<long>(s + hdr), fv.begin() + static_cast<long>(s + size)),
                 "inside an FV_IMAGE section", depth + 1);
        }
        s = (s + size + 3) & ~std::size_t{3};  // sections are 4-byte aligned
    }
    return name;
}

void walk_volume(const Bytes& b, std::size_t fv0, int depth)
{
    const std::uint64_t length = u64(b, fv0 + 32);
    const std::uint32_t header_len = u16(b, fv0 + 48);
    const std::uint32_t ext = u16(b, fv0 + 52);
    std::uint32_t sum = 0;               // the header's 16-bit words must add up to zero
    for (std::size_t i = 0; i < header_len; i += 2) {
        sum = (sum + u16(b, fv0 + i)) & 0xffff;
    }
    std::printf("%*sFV at offset 0x%zx, length 0x%llx, header %u bytes, header checksum %s\n", depth * 4, "",
                fv0, static_cast<unsigned long long>(length), header_len, sum == 0 ? "ok" : "BAD");
    std::size_t off = header_len;
    if (ext != 0) {                      // the extended header holds the volume's own name GUID
        off = ext + u32(b, fv0 + ext + 16);
    }
    std::map<std::string, int> counts;
    const Bytes fv(b.begin() + static_cast<long>(fv0), b.begin() + static_cast<long>(fv0 + length));
    for (off = (off + 7) & ~std::size_t{7}; off + 24 <= fv.size(); off = (off + 7) & ~std::size_t{7}) {
        bool erased = true;
        for (std::size_t i = 0; i < 24; ++i) {
            erased = erased && fv[off + i] == 0xff;
        }
        if (erased) {
            break;                       // the rest of the volume is free (erased flash)
        }
        const unsigned type = fv[off + 18];
        std::size_t size = u24(fv, off + 20);
        std::size_t hdr = 24;
        if (size == 0xffffff) {          // large file: 64-bit size after the header
            size = u64(fv, off + 24);
            hdr = 32;
        }
        if (size < hdr || off + size > fv.size()) {
            std::printf("%*s  file header at 0x%zx is damaged: stopping\n", depth * 4, "", off);
            break;
        }
        if (type != 0xf0) {
            const std::string name = sections(fv, off + hdr, off + size, -1);
            std::printf("%*s  %-22s %8zu bytes  %s  %s\n", depth * 4, "", file_type(type), size,
                        guid(fv, off).substr(0, 8).c_str(), name.c_str());
            sections(fv, off + hdr, off + size, depth);  // open what is packed inside
            ++counts[file_type(type)];
        }
        off += size;
    }
    std::printf("%*s  summary:", depth * 4, "");
    for (const auto& [type, n] : counts) {
        std::printf(" %s %d;", type.c_str(), n);
    }
    std::printf("\n");
}

void scan(const Bytes& data, const std::string& where, int depth)
{
    std::printf("%*s-- scanning %zu bytes %s\n", depth * 4, "", data.size(), where.c_str());
    for (std::size_t o = 0; o + 56 <= data.size(); o += 8) {
        if (std::memcmp(&data[o + 40], "_FVH", 4) == 0 && u64(data, o + 32) <= data.size() - o &&
            u64(data, o + 32) >= 56) {
            walk_volume(data, o, depth);
            o += u64(data, o + 32) - 8;
        }
    }
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::printf("usage: fvscan <firmware image>\n");
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::printf("cannot open %s\n", argv[1]);
        return 1;
    }
    const Bytes image((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    scan(image, std::string("in ") + argv[1], 0);
    return 0;
}
