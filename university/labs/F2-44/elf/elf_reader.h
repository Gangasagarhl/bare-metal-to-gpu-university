// elf_reader.h: a small, bounds-checked reader for 64-bit little-endian ELF files
// (curriculum milestone P2). Rules it follows:
//   - no exceptions and no memory allocation inside the parser, so the same code can later
//     run in a freestanding loader (milestone A3) or kernel (B13);
//   - every offset and size read from the file is checked against the file size before use,
//     with overflow-safe arithmetic, so a malformed file gives an Error, never a crash;
//   - field layouts follow the ELF specification (System V gABI); see chapter F2-44, D1.
#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

namespace elf {

static_assert(std::endian::native == std::endian::little, "reader assumes a little-endian host");

struct Ehdr {                                   // ELF header: 64 bytes in ELF64
    std::array<std::uint8_t, 16> ident;         // magic, class, data, version, OS/ABI, ...
    std::uint16_t type;                         // REL, EXEC, DYN, CORE
    std::uint16_t machine;                      // target ISA
    std::uint32_t version;
    std::uint64_t entry;                        // virtual address of the first instruction
    std::uint64_t phoff;                        // file offset of the program header table
    std::uint64_t shoff;                        // file offset of the section header table
    std::uint32_t flags;
    std::uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
};
struct Shdr {                                   // section header: 64 bytes (the linker's view)
    std::uint32_t name, type;
    std::uint64_t flags, addr, offset, size;
    std::uint32_t link, info;
    std::uint64_t addralign, entsize;
};
struct Phdr {                                   // program header: 56 bytes (the loader's view)
    std::uint32_t type, flags;
    std::uint64_t offset, vaddr, paddr, filesz, memsz, align;
};
struct Sym {                                    // symbol table entry: 24 bytes
    std::uint32_t name;
    std::uint8_t info, other;
    std::uint16_t shndx;
    std::uint64_t value, size;
};
static_assert(sizeof(Ehdr) == 64 && offsetof(Ehdr, entry) == 24 && offsetof(Ehdr, shstrndx) == 62);
static_assert(sizeof(Shdr) == 64 && offsetof(Shdr, offset) == 24 && offsetof(Shdr, entsize) == 56);
static_assert(sizeof(Phdr) == 56 && offsetof(Phdr, offset) == 8 && offsetof(Phdr, align) == 48);
static_assert(sizeof(Sym) == 24 && offsetof(Sym, shndx) == 6 && offsetof(Sym, value) == 8);

inline constexpr std::uint32_t SHT_SYMTAB = 2, SHT_STRTAB = 3, SHT_NOBITS = 8, SHT_DYNSYM = 11;
inline constexpr std::uint16_t SHN_XINDEX = 0xffff;

enum class Error {
    ok, too_small, bad_magic, not_elf64, not_little_endian, bad_version, bad_header_size,
    bad_entry_size, phdrs_outside_file, shdrs_outside_file, bad_shstrndx, index_out_of_range,
    section_outside_file, not_a_string_table, string_outside_table, unterminated_string,
    not_a_symbol_table, bad_symbol_entry_size
};

inline const char* message(Error e)
{
    switch (e) {
    case Error::ok: return "ok";
    case Error::too_small: return "file smaller than an ELF64 header";
    case Error::bad_magic: return "no ELF magic number";
    case Error::not_elf64: return "not ELFCLASS64 (only 64-bit files are supported)";
    case Error::not_little_endian: return "not little-endian (ELFDATA2LSB)";
    case Error::bad_version: return "unknown ELF version";
    case Error::bad_header_size: return "e_ehsize is not 64";
    case Error::bad_entry_size: return "e_phentsize or e_shentsize does not match ELF64";
    case Error::phdrs_outside_file: return "program header table lies outside the file";
    case Error::shdrs_outside_file: return "section header table lies outside the file";
    case Error::bad_shstrndx: return "e_shstrndx does not name a section";
    case Error::index_out_of_range: return "index out of range";
    case Error::section_outside_file: return "section contents lie outside the file";
    case Error::not_a_string_table: return "linked section is not a string table";
    case Error::string_outside_table: return "string offset outside its string table";
    case Error::unterminated_string: return "string not terminated inside its table";
    case Error::not_a_symbol_table: return "section is not a symbol table";
    case Error::bad_symbol_entry_size: return "symbol table sh_entsize is not 24";
    }
    return "unknown error";
}

class File {
public:
    // Validates the header and both header tables. On success, every later accessor that
    // returns Error::ok has checked its own offsets too.
    static Error open(std::span<const std::byte> bytes, File& out)
    {
        File f;
        f.bytes_ = bytes;
        if (bytes.size() < sizeof(Ehdr)) return Error::too_small;
        std::memcpy(&f.eh_, bytes.data(), sizeof(Ehdr));
        const auto& id = f.eh_.ident;
        if (id[0] != 0x7f || id[1] != 'E' || id[2] != 'L' || id[3] != 'F') return Error::bad_magic;
        if (id[4] != 2) return Error::not_elf64;            // EI_CLASS: 2 = ELFCLASS64
        if (id[5] != 1) return Error::not_little_endian;    // EI_DATA: 1 = ELFDATA2LSB
        if (id[6] != 1 || f.eh_.version != 1) return Error::bad_version;
        if (f.eh_.ehsize != sizeof(Ehdr)) return Error::bad_header_size;
        if (f.eh_.phnum != 0 && f.eh_.phentsize != sizeof(Phdr)) return Error::bad_entry_size;
        if (f.eh_.shoff != 0 && f.eh_.shentsize != sizeof(Shdr)) return Error::bad_entry_size;
        if (!f.inFile(f.eh_.phoff, std::uint64_t{f.eh_.phnum} * sizeof(Phdr))) {
            return Error::phdrs_outside_file;
        }

        // Section count and string-table index, including the "extended numbering" escape:
        // if the real values do not fit in 16 bits, they are stored in section header 0.
        f.shnum_ = f.eh_.shnum;
        f.shstrndx_ = f.eh_.shstrndx;
        if (f.eh_.shoff != 0) {
            if (!f.inFile(f.eh_.shoff, sizeof(Shdr))) return Error::shdrs_outside_file;
            Shdr first{};
            std::memcpy(&first, bytes.data() + f.eh_.shoff, sizeof(Shdr));
            if (f.shnum_ == 0) f.shnum_ = first.size;
            if (f.shstrndx_ == SHN_XINDEX) f.shstrndx_ = first.link;
            const std::uint64_t maxEntries = bytes.size() / sizeof(Shdr);
            if (f.shnum_ > maxEntries || !f.inFile(f.eh_.shoff, f.shnum_ * sizeof(Shdr))) {
                return Error::shdrs_outside_file;
            }
        } else {
            f.shnum_ = 0;
        }
        if (f.shnum_ != 0 && f.shstrndx_ >= f.shnum_) return Error::bad_shstrndx;
        out = f;
        return Error::ok;
    }

    const Ehdr& header() const { return eh_; }
    std::uint64_t sectionCount() const { return shnum_; }
    std::uint64_t segmentCount() const { return eh_.phnum; }
    std::uint64_t shstrndx() const { return shstrndx_; }

    Error section(std::uint64_t i, Shdr& out) const
    {
        if (i >= shnum_) return Error::index_out_of_range;
        std::memcpy(&out, bytes_.data() + eh_.shoff + i * sizeof(Shdr), sizeof(Shdr));
        return Error::ok;
    }

    Error segment(std::uint64_t i, Phdr& out) const
    {
        if (i >= eh_.phnum) return Error::index_out_of_range;
        std::memcpy(&out, bytes_.data() + eh_.phoff + i * sizeof(Phdr), sizeof(Phdr));
        return Error::ok;
    }

    // The bytes of a section; empty for SHT_NOBITS (.bss occupies no file space).
    Error contents(const Shdr& s, std::span<const std::byte>& out) const
    {
        if (s.type == SHT_NOBITS) { out = {}; return Error::ok; }
        if (!inFile(s.offset, s.size)) return Error::section_outside_file;
        out = bytes_.subspan(s.offset, s.size);
        return Error::ok;
    }

    // A NUL-terminated string at 'offset' inside string-table section 'tableIndex'.
    Error string(std::uint64_t tableIndex, std::uint64_t offset, std::string_view& out) const
    {
        Shdr t{};
        if (Error e = section(tableIndex, t); e != Error::ok) return e;
        if (t.type != SHT_STRTAB) return Error::not_a_string_table;
        std::span<const std::byte> data;
        if (Error e = contents(t, data); e != Error::ok) return e;
        if (offset >= data.size()) return Error::string_outside_table;
        const auto* begin = reinterpret_cast<const char*>(data.data()) + offset;
        const auto* nul = static_cast<const char*>(std::memchr(begin, 0, data.size() - offset));
        if (nul == nullptr) return Error::unterminated_string;
        out = std::string_view(begin, static_cast<std::size_t>(nul - begin));
        return Error::ok;
    }

    Error sectionName(const Shdr& s, std::string_view& out) const
    {
        if (shnum_ == 0) return Error::bad_shstrndx;
        return string(shstrndx_, s.name, out);
    }

    Error symbolCount(const Shdr& table, std::uint64_t& count) const
    {
        if (table.type != SHT_SYMTAB && table.type != SHT_DYNSYM) return Error::not_a_symbol_table;
        if (table.entsize != sizeof(Sym)) return Error::bad_symbol_entry_size;
        if (!inFile(table.offset, table.size)) return Error::section_outside_file;
        count = table.size / sizeof(Sym);
        return Error::ok;
    }

    Error symbol(const Shdr& table, std::uint64_t i, Sym& out) const
    {
        std::uint64_t n = 0;
        if (Error e = symbolCount(table, n); e != Error::ok) return e;
        if (i >= n) return Error::index_out_of_range;
        std::memcpy(&out, bytes_.data() + table.offset + i * sizeof(Sym), sizeof(Sym));
        return Error::ok;
    }

private:
    // True when [off, off + len) lies inside the file, without overflowing.
    bool inFile(std::uint64_t off, std::uint64_t len) const
    {
        return off <= bytes_.size() && len <= bytes_.size() - off;
    }

    std::span<const std::byte> bytes_;
    Ehdr eh_{};
    std::uint64_t shnum_ = 0;
    std::uint64_t shstrndx_ = 0;
};

}  // namespace elf
