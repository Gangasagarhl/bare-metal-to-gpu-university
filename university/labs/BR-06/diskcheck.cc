// diskcheck.cc - BR-06: a colleague's volume checker. It reads an ext2 superblock
// (1024 bytes on stdin) and decides whether the volume may be mounted.
//   default build:   copies the bytes into a C struct and uses its fields directly;
//   -DFIXED build:   reads every field with an explicit little-endian helper.
// ext2 stores its numbers little-endian on disk. Field offsets: the ext2 superblock layout
// (title only, pending verification); the run cross-checks them against dumpe2fs.
#include "ustart.h"

using u8 = __UINT8_TYPE__;
using u16 = __UINT16_TYPE__;
using u32 = __UINT32_TYPE__;

namespace {

#ifndef FIXED
struct Ext2SuperPrefix {          // the first 58 bytes, as the colleague's header declares them
    u32 s_inodes_count;           // offset 0
    u32 s_blocks_count;           // 4
    u32 s_r_blocks_count;         // 8
    u32 s_free_blocks_count;      // 12
    u32 s_free_inodes_count;      // 16
    u32 s_first_data_block;       // 20
    u32 s_log_block_size;         // 24: block size = 1024 << this
    u32 s_log_frag_size;          // 28
    u32 s_blocks_per_group;       // 32
    u32 s_frags_per_group;        // 36
    u32 s_inodes_per_group;       // 40
    u32 s_mtime;                  // 44
    u32 s_wtime;                  // 48
    u16 s_mnt_count;              // 52
    u16 s_max_mnt_count;          // 54
    u16 s_magic;                  // 56: 0xEF53
};

#else
u16 le16(const u8* p) { return static_cast<u16>(p[0] | (p[1] << 8)); }
u32 le32(const u8* p) { return u32(p[0]) | u32(p[1]) << 8 | u32(p[2]) << 16 | u32(p[3]) << 24; }
#endif

void put(const char* s)
{
    long n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    sys::write(1, s, n);
}

void num(u32 v)
{
    char b[12];
    int i = 11;
    b[i] = '\0';
    do {
        b[--i] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    put(b + i);
}

void hex16(u16 v)
{
    char b[7] = {'0', 'x', 0, 0, 0, 0, 0};
    for (int i = 0; i < 4; ++i) {
        b[2 + i] = "0123456789abcdef"[(v >> (12 - 4 * i)) & 0xf];
    }
    put(b);
}

// Prints one report line; returns true if the volume would be accepted.
bool report(const char* how, u16 magic, u32 blocks, u32 inodes, u32 log_bs)
{
    put(how);
    put(" magic ");
    hex16(magic);
    put("  blocks ");
    num(blocks);
    put("  inodes ");
    num(inodes);
    put("  s_log_block_size ");
    num(log_bs);
    if (magic != 0xef53) {
        put("\n  -> not an ext2 volume, refusing to mount\n");
        return false;
    }
    put("\n  -> ext2, block size ");
    num(log_bs < 8 ? 1024u << log_bs : 0);
    put(", ok to mount\n");
    return true;
}

} // namespace

extern "C" [[noreturn]] void umain()
{
    alignas(8) static u8 sb[1024];
    long got = 0;
    while (got < 1024) {
        const long n = sys::read(0, sb + got, 1024 - got);
        if (n <= 0) {
            break;
        }
        got += n;
    }
    put("diskcheck on " ARCH_NAME "\n");
    if (got != 1024) {
        put("short read\n");
        sys::exit(2);
    }
#ifndef FIXED
    Ext2SuperPrefix s;
    memcpy(&s, sb, sizeof s);                 // bytes taken as native-order numbers
    const bool ok = report("  superblock:", s.s_magic, s.s_blocks_count, s.s_inodes_count, s.s_log_block_size);
#else
    const bool ok = report("  superblock:", le16(sb + 56), le32(sb + 4), le32(sb + 0), le32(sb + 24));
#endif
    sys::exit(ok ? 0 : 1);
}
