// elf_tests.cpp: host unit tests for elf_reader.h. A tiny valid ELF64 file is built in memory,
// then broken on purpose in the ways milestone P2 names (truncated file, bad offsets) and a
// few more. Every broken file must be rejected with an Error, never crash (the run uses
// AddressSanitizer and UndefinedBehaviorSanitizer).
#include "elf/elf_reader.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace {

int failures = 0;

void check(bool ok, const char* what)
{
    std::printf("%-58s %s\n", what, ok ? "pass" : "FAIL");
    if (!ok) ++failures;
}

template <typename T>
void put(std::vector<std::byte>& f, std::size_t off, const T& v)
{
    std::memcpy(f.data() + off, &v, sizeof v);
}

// Layout: ELF header | ".shstrtab" contents | 3 section headers (NULL, .shstrtab, .text).
std::vector<std::byte> tinyElf()
{
    const char names[] = "\0.shstrtab\0.text";          // offsets 0, 1, 11
    const std::size_t namesOff = 64, shOff = 96;
    std::vector<std::byte> f(shOff + 3 * sizeof(elf::Shdr));
    elf::Ehdr h{};
    h.ident = {0x7f, 'E', 'L', 'F', 2, 1, 1};
    h.type = 1;           // relocatable
    h.machine = 62;       // x86-64
    h.version = 1;
    h.shoff = shOff;
    h.ehsize = 64;
    h.shentsize = 64;
    h.shnum = 3;
    h.shstrndx = 1;
    put(f, 0, h);
    std::memcpy(f.data() + namesOff, names, sizeof names);
    elf::Shdr strtab{};
    strtab.name = 1;
    strtab.type = elf::SHT_STRTAB;
    strtab.offset = namesOff;
    strtab.size = sizeof names;
    elf::Shdr text{};
    text.name = 11; text.type = 1; text.flags = 6; text.offset = 64; text.size = 0;
    put(f, shOff + 64, strtab);
    put(f, shOff + 128, text);
    return f;
}

elf::Error openErr(const std::vector<std::byte>& f)
{
    elf::File file;
    return elf::File::open(f, file);
}

// Opens the file and walks every section name; returns the first error met.
elf::Error walk(const std::vector<std::byte>& f)
{
    elf::File file;
    if (elf::Error e = elf::File::open(f, file); e != elf::Error::ok) return e;
    for (std::uint64_t i = 0; i < file.sectionCount(); ++i) {
        elf::Shdr s{};
        if (elf::Error e = file.section(i, s); e != elf::Error::ok) return e;
        std::string_view name;
        if (elf::Error e = file.sectionName(s, name); e != elf::Error::ok) return e;
    }
    return elf::Error::ok;
}

}  // namespace

int main()
{
    const auto good = tinyElf();
    check(walk(good) == elf::Error::ok, "valid tiny file: opens and all names resolve");
    {
        elf::File file;
        elf::File::open(good, file);
        elf::Shdr s{};
        std::string_view name;
        file.section(2, s);
        file.sectionName(s, name);
        check(name == ".text", "section 2 is named .text");
    }

    int truncOk = 0;
    for (std::size_t n = 0; n < good.size(); ++n) {
        std::vector<std::byte> cut(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(n));
        if (walk(cut) != elf::Error::ok) ++truncOk;
    }
    std::printf("truncated copies rejected: %d of %zu\n", truncOk, good.size());
    check(truncOk == static_cast<int>(good.size()), "every truncation (0..size-1 bytes) rejected");

    auto f = good; f[1] = std::byte{'X'};
    check(openErr(f) == elf::Error::bad_magic, "bad magic -> bad_magic");
    f = good; f[4] = std::byte{1};
    check(openErr(f) == elf::Error::not_elf64, "ELFCLASS32 -> not_elf64");
    f = good; f[5] = std::byte{2};
    check(openErr(f) == elf::Error::not_little_endian, "big-endian -> not_little_endian");
    f = good; put(f, 40, std::uint64_t{0xfffffffffffffff0ULL});
    check(openErr(f) == elf::Error::shdrs_outside_file, "e_shoff near 2^64 (overflow) -> rejected");
    f = good; put(f, 60, std::uint16_t{60000});
    check(openErr(f) == elf::Error::shdrs_outside_file, "e_shnum = 60000 -> shdrs_outside_file");
    f = good; put(f, 62, std::uint16_t{7});
    check(openErr(f) == elf::Error::bad_shstrndx, "e_shstrndx = 7 of 3 -> bad_shstrndx");
    f = good;
    put(f, 56, std::uint16_t{1});      // e_phnum
    put(f, 54, std::uint16_t{56});     // e_phentsize
    put(f, 32, std::uint64_t{260});    // e_phoff: 260 + 56 > 288 bytes
    check(openErr(f) == elf::Error::phdrs_outside_file, "e_phoff past the end -> rejected");
    f = good; put(f, 58, std::uint16_t{40});
    check(openErr(f) == elf::Error::bad_entry_size, "e_shentsize = 40 -> bad_entry_size");
    f = good; put(f, 96 + 64 + 24, std::uint64_t{1000});            // .shstrtab sh_offset
    check(walk(f) == elf::Error::section_outside_file, ".shstrtab offset past the end -> rejected");
    f = good; put(f, 96 + 64 + 32, std::uint64_t{0xffffffffffffff00ULL});   // sh_size
    check(walk(f) == elf::Error::section_outside_file, ".shstrtab size near 2^64 -> rejected");
    f = good; put(f, 96 + 128, std::uint32_t{500});                   // .text sh_name
    check(walk(f) == elf::Error::string_outside_table, "section name offset 500 -> rejected");
    f = good; f[64 + 16] = std::byte{'X'};                              // remove the final NUL
    check(walk(f) == elf::Error::unterminated_string, "last name without NUL -> rejected");
    f = good; put(f, 96 + 64 + 4, std::uint32_t{1});                  // .shstrtab type PROGBITS
    check(walk(f) == elf::Error::not_a_string_table, "shstrtab not STRTAB -> rejected");

    std::printf("%s: %d failure(s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                failures);
    return failures == 0 ? 0 : 1;
}
