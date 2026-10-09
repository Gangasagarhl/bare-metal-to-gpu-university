// reloc_check.cpp: redo the linker's arithmetic for one relocation and compare it with the
// four bytes the linker really wrote. Usage: reloc_check TYPE S A P BYTES
//   TYPE  R_X86_64_PC32 or R_X86_64_PLT32 (both computed here as S + A - P; see the chapter)
//   S     final address of the symbol (hex), A the addend (decimal, may be negative),
//   P     final address of the 4-byte field being patched (hex),
//   BYTES the field's 4 bytes as read from the file (run.sh uses od), e.g. "38 00 00 00"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 6) {
        std::fprintf(stderr, "usage: reloc_check TYPE S A P \"b0 b1 b2 b3\"\n");
        return 2;
    }
    const std::string type = argv[1];
    const std::int64_t s = static_cast<std::int64_t>(std::strtoull(argv[2], nullptr, 16));
    const std::int64_t a = std::strtoll(argv[3], nullptr, 10);
    const std::int64_t p = static_cast<std::int64_t>(std::strtoull(argv[4], nullptr, 16));
    const std::int64_t value = s + a - p;

    std::uint32_t field = 0;
    std::istringstream in(argv[5]);
    for (int i = 0; i < 4; ++i) {
        unsigned b = 0;
        in >> std::hex >> b;
        field |= static_cast<std::uint32_t>(b & 0xff) << (8 * i);   // little-endian
    }
    const auto written = static_cast<std::int32_t>(field);

    std::printf("%s: S = 0x%llx, A = %lld, P = 0x%llx\n", type.c_str(),
                static_cast<unsigned long long>(s), static_cast<long long>(a),
                static_cast<unsigned long long>(p));
    std::printf("  S + A - P = %lld (0x%llx as a 32-bit field: 0x%08x)\n",
                static_cast<long long>(value), static_cast<unsigned long long>(value),
                static_cast<std::uint32_t>(value));
    std::printf("  linker wrote %d (bytes %s)\n", written, argv[5]);
    const bool fits = value >= INT32_MIN && value <= INT32_MAX;
    std::printf("  fits in a signed 32-bit field: %s; match: %s\n", fits ? "yes" : "no",
                (fits && written == value) ? "YES" : "NO");
    return (fits && written == value) ? 0 : 1;
}
