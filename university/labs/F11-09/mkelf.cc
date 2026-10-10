// mkelf.cc - write a minimal, well-formed ELF64 seed for the fuzzer. Starting
// from a small valid input (rather than a 16 KB real binary) keeps the crashing
// inputs small, so minimisation is fast and the result is easy to read.
// Layout: 64-byte ELF header, then two 64-byte section headers, then a small
// section-name string table. Usage: ./mkelf <out>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>

static void put16(std::vector<unsigned char>& b, size_t at, uint16_t v)
{ b[at] = v & 0xff; b[at + 1] = (v >> 8) & 0xff; }
static void put32(std::vector<unsigned char>& b, size_t at, uint32_t v)
{ for (int i = 0; i < 4; ++i) b[at + i] = (v >> (8 * i)) & 0xff; }
static void put64(std::vector<unsigned char>& b, size_t at, uint64_t v)
{ for (int i = 0; i < 8; ++i) b[at + i] = (v >> (8 * i)) & 0xff; }

int main(int argc, char** argv)
{
    if (argc < 2) return 2;
    const unsigned SH = 64;                 // section header entry size
    const unsigned nsec = 2;
    size_t shoff = 64;
    size_t stroff = shoff + nsec * SH;      // string table right after the headers
    const char strtab[] = "\0.shstrtab";    // index 0 empty, index 1 ".shstrtab"
    size_t total = stroff + sizeof strtab;

    std::vector<unsigned char> b(total, 0);
    b[0] = 0x7f; b[1] = 'E'; b[2] = 'L'; b[3] = 'F';
    b[4] = 2; b[5] = 1; b[6] = 1;           // ELFCLASS64, little-endian, version 1
    put16(b, 16, 1);                        // e_type = ET_REL
    put16(b, 18, 62);                       // e_machine = x86-64
    put64(b, 40, shoff);                    // e_shoff
    put16(b, 58, SH);                       // e_shentsize
    put16(b, 60, nsec);                     // e_shnum
    put16(b, 62, 1);                        // e_shstrndx = section 1

    // Section 0: the reserved null section (sh_name 0).
    // Section 1: the string table itself.
    size_t s1 = shoff + SH;
    put32(b, s1 + 0, 1);                    // sh_name -> ".shstrtab"
    put32(b, s1 + 4, 3);                    // sh_type = SHT_STRTAB
    put64(b, s1 + 24, stroff);              // sh_offset
    put64(b, s1 + 32, sizeof strtab);       // sh_size

    std::memcpy(&b[stroff], strtab, sizeof strtab);

    FILE* o = std::fopen(argv[1], "wb");
    if (!o) return 2;
    std::fwrite(b.data(), 1, b.size(), o);
    std::fclose(o);
    std::printf("wrote %zu-byte ELF seed\n", total);
    return 0;
}
