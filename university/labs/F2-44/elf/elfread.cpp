// elfread.cpp: print an ELF64 file's header, section headers, program headers and symbols in
// one line per item ("canonical" form), so that the lab can compare them with readelf.
// Usage: elfread FILE      Exit code 0 = read completely, 1 = rejected (message on stderr).
#include "elf_reader.h"

#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

// Names exactly as readelf prints them, for the values met in the lab's binaries. Unknown
// values print as hexadecimal numbers, so a missing name shows up as a difference, not a lie.
std::string sectionType(std::uint32_t t)
{
    switch (t) {
    case 0: return "NULL";          case 1: return "PROGBITS";     case 2: return "SYMTAB";
    case 3: return "STRTAB";        case 4: return "RELA";         case 5: return "HASH";
    case 6: return "DYNAMIC";       case 7: return "NOTE";         case 8: return "NOBITS";
    case 9: return "REL";           case 11: return "DYNSYM";      case 14: return "INIT_ARRAY";
    case 15: return "FINI_ARRAY";   case 16: return "PREINIT_ARRAY"; case 17: return "GROUP";
    case 0x6ffffff6: return "GNU_HASH";   case 0x6ffffffd: return "VERDEF";
    case 0x6ffffffe: return "VERNEED";    case 0x6fffffff: return "VERSYM";
    case 0x70000003: return "AARCH64_ATTRIBUTES";
    default: break;
    }
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%x", t);
    return buf;
}

std::string segmentType(std::uint32_t t)
{
    switch (t) {
    case 0: return "NULL";    case 1: return "LOAD";   case 2: return "DYNAMIC";
    case 3: return "INTERP";  case 4: return "NOTE";   case 5: return "SHLIB";
    case 6: return "PHDR";    case 7: return "TLS";
    case 0x6474e550: return "GNU_EH_FRAME"; case 0x6474e551: return "GNU_STACK";
    case 0x6474e552: return "GNU_RELRO";    case 0x6474e553: return "GNU_PROPERTY";
    default: break;
    }
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%x", t);
    return buf;
}

std::string machine(std::uint16_t m)
{
    switch (m) {
    case 3: return "Intel 80386";
    case 62: return "Advanced Micro Devices X86-64";
    case 183: return "AArch64";
    case 243: return "RISC-V";
    default: return "machine " + std::to_string(m);
    }
}

// Section flag letters in readelf's order: W A X M S I L O G T C x o E D l p
std::string sectionFlags(std::uint64_t f)
{
    static constexpr struct { std::uint64_t bit; char letter; } table[] = {
        {0x1, 'W'}, {0x2, 'A'}, {0x4, 'X'}, {0x10, 'M'}, {0x20, 'S'}, {0x40, 'I'}, {0x80, 'L'},
        {0x100, 'O'}, {0x200, 'G'}, {0x400, 'T'}, {0x800, 'C'}, {0x80000000, 'E'}};
    std::string s;
    for (const auto& e : table) {
        if (f & e.bit) s += e.letter;
    }
    return s.empty() ? "-" : s;
}

std::string segmentFlags(std::uint32_t f)
{
    std::string s;
    s += (f & 4) ? 'R' : '-';
    s += (f & 2) ? 'W' : '-';
    s += (f & 1) ? 'E' : '-';
    return s;
}

const char* symType(unsigned t)
{
    static const char* names[] = {"NOTYPE", "OBJECT", "FUNC", "SECTION", "FILE", "COMMON", "TLS"};
    if (t < 7) return names[t];
    return t == 10 ? "IFUNC" : "?";
}
const char* symBind(unsigned b)
{
    static const char* names[] = {"LOCAL", "GLOBAL", "WEAK"};
    if (b < 3) return names[b];
    return b == 10 ? "UNIQUE" : "?";
}
const char* symVis(unsigned v)
{
    static const char* names[] = {"DEFAULT", "INTERNAL", "HIDDEN", "PROTECTED"};
    return names[v & 3];
}

std::string symIndex(std::uint16_t n)
{
    if (n == 0) return "UND";
    if (n == 0xfff1) return "ABS";
    if (n == 0xfff2) return "COM";
    return std::to_string(n);
}

int fail(const char* what, elf::Error e)
{
    std::fprintf(stderr, "elfread: %s: %s\n", what, elf::message(e));
    return 1;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: elfread FILE\n");
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "elfread: cannot open %s\n", argv[1]);
        return 2;
    }
    const std::vector<char> raw{std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>()};
    const std::span<const std::byte> bytes(reinterpret_cast<const std::byte*>(raw.data()),
                                           raw.size());

    elf::File f;
    if (elf::Error e = elf::File::open(bytes, f); e != elf::Error::ok) return fail("header", e);
    const elf::Ehdr& h = f.header();
    std::printf("H type=%u machine=%s entry=0x%" PRIx64 " phoff=%" PRIu64 " shoff=%" PRIu64
                " flags=0x%x ehsize=%u phentsize=%u phnum=%u shentsize=%u shnum=%u shstrndx=%u\n",
                h.type, machine(h.machine).c_str(), h.entry, h.phoff, h.shoff, h.flags, h.ehsize,
                h.phentsize, h.phnum, h.shentsize, h.shnum, h.shstrndx);

    for (std::uint64_t i = 0; i < f.sectionCount(); ++i) {
        elf::Shdr s{};
        if (elf::Error e = f.section(i, s); e != elf::Error::ok) return fail("section header", e);
        std::string_view name;
        if (elf::Error e = f.sectionName(s, name); e != elf::Error::ok) {
            return fail("section name", e);
        }
        std::printf("S %" PRIu64 " %.*s %s %016" PRIx64 " %06" PRIx64 " %06" PRIx64 " %02" PRIx64
                    " %s %u %u %" PRIu64 "\n",
                    i, static_cast<int>(name.size()), name.data(), sectionType(s.type).c_str(),
                    s.addr, s.offset, s.size, s.entsize, sectionFlags(s.flags).c_str(), s.link,
                    s.info, s.addralign);
    }

    for (std::uint64_t i = 0; i < f.segmentCount(); ++i) {
        elf::Phdr p{};
        if (elf::Error e = f.segment(i, p); e != elf::Error::ok) return fail("program header", e);
        std::printf("P %s 0x%06" PRIx64 " 0x%016" PRIx64 " 0x%016" PRIx64 " 0x%06" PRIx64
                    " 0x%06" PRIx64 " %s 0x%" PRIx64 "\n",
                    segmentType(p.type).c_str(), p.offset, p.vaddr, p.paddr, p.filesz, p.memsz,
                    segmentFlags(p.flags).c_str(), p.align);
    }

    for (std::uint64_t i = 0; i < f.sectionCount(); ++i) {
        elf::Shdr t{};
        if (elf::Error e = f.section(i, t); e != elf::Error::ok) return fail("section header", e);
        if (t.type != elf::SHT_SYMTAB && t.type != elf::SHT_DYNSYM) continue;
        std::string_view tname;
        if (elf::Error e = f.sectionName(t, tname); e != elf::Error::ok) {
            return fail("table name", e);
        }
        std::uint64_t n = 0;
        if (elf::Error e = f.symbolCount(t, n); e != elf::Error::ok) return fail("symbol table", e);
        for (std::uint64_t k = 0; k < n; ++k) {
            elf::Sym y{};
            if (elf::Error e = f.symbol(t, k, y); e != elf::Error::ok) return fail("symbol", e);
            std::string_view name;
            if (elf::Error e = f.string(t.link, y.name, name); e != elf::Error::ok) {
                return fail("symbol name", e);
            }
            // readelf shows a section symbol (type SECTION, empty name) under the name of the
            // section it stands for; do the same so the two outputs can be compared.
            if ((y.info & 0xf) == 3 && name.empty() && y.shndx != 0 && y.shndx < 0xff00) {
                elf::Shdr target{};
                if (elf::Error e = f.section(y.shndx, target); e != elf::Error::ok) {
                    return fail("symbol section", e);
                }
                if (elf::Error e = f.sectionName(target, name); e != elf::Error::ok) {
                    return fail("symbol section name", e);
                }
            }
            std::printf("Y %.*s %" PRIu64 " %016" PRIx64 " %" PRIu64 " %s %s %s %s %.*s\n",
                        static_cast<int>(tname.size()), tname.data(), k, y.value, y.size,
                        symType(y.info & 0xf), symBind(y.info >> 4), symVis(y.other),
                        symIndex(y.shndx).c_str(), static_cast<int>(name.size()), name.data());
        }
    }
    return 0;
}
