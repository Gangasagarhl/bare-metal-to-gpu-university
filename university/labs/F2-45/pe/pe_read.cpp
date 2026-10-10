// pe_read.cpp: a small bounds-checked reader for PE32+ images and COFF object files
// (the PE/COFF half of curriculum milestone P2). It prints one "Key value" line per field,
// using the field names that llvm-readobj prints, so the lab can compare the two.
// Usage: pe_read FILE      Exit code 0 = read completely, 1 = rejected.
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace {

std::span<const std::uint8_t> file;   // the whole file

// Overflow-safe check that [off, off + len) is inside the file.
bool inFile(std::uint64_t off, std::uint64_t len)
{
    return off <= file.size() && len <= file.size() - off;
}

template <typename T>
bool get(std::uint64_t off, T& out)   // little-endian fields, as in the file
{
    if (!inFile(off, sizeof(T))) return false;
    std::memcpy(&out, file.data() + off, sizeof(T));
    return true;
}

int reject(const char* why)
{
    std::fprintf(stderr, "pe_read: rejected: %s\n", why);
    return 1;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: pe_read FILE\n");
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    const std::vector<char> raw{std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>()};
    file = std::span(reinterpret_cast<const std::uint8_t*>(raw.data()), raw.size());

    // An image starts with the MS-DOS stub header ("MZ"); its field at offset 0x3c gives the
    // file offset of the "PE\0\0" signature, which the COFF file header follows. A COFF object
    // file has no stub and no signature: it starts directly with the COFF file header.
    std::uint64_t coff = 0;
    bool image = false;
    std::uint16_t mz = 0;
    if (get(0, mz) && mz == 0x5a4d) {   // "MZ"
        std::uint32_t lfanew = 0, sig = 0;
        if (!get(0x3c, lfanew)) return reject("truncated MS-DOS header");
        if (!get(lfanew, sig) || sig != 0x00004550) return reject("no PE\\0\\0 signature");
        coff = std::uint64_t{lfanew} + 4;
        image = true;
    }
    std::uint16_t machine = 0, nsect = 0, optSize = 0, chars = 0;
    std::uint32_t stamp = 0, symPtr = 0, nsym = 0;
    if (!get(coff + 0, machine) || !get(coff + 2, nsect) || !get(coff + 4, stamp) ||
        !get(coff + 8, symPtr) || !get(coff + 12, nsym) || !get(coff + 16, optSize) ||
        !get(coff + 18, chars)) {
        return reject("truncated COFF file header");
    }
    std::printf("Format %s\n", image ? "image" : "object");
    std::printf("Machine 0x%x\nSectionCount %u\nTimeDateStamp 0x%x\nPointerToSymbolTable 0x%x\n"
                "SymbolCount %u\nOptionalHeaderSize %u\nCharacteristics 0x%x\n",
                machine, nsect, stamp, symPtr, nsym, optSize, chars);

    const std::uint64_t opt = coff + 20;
    std::uint32_t relocRva = 0, relocSize = 0;
    if (optSize != 0) {
        std::uint16_t magic = 0, subsystem = 0, dllChars = 0;
        std::uint32_t entry = 0, sectAlign = 0, fileAlign = 0, imageSize = 0, headerSize = 0,
                      nDirs = 0;
        std::uint64_t base = 0;
        if (!get(opt + 0, magic)) return reject("truncated optional header");
        if (magic != 0x20b) return reject("optional header is not PE32+ (magic 0x20b)");
        if (optSize < 112 || !inFile(opt, optSize)) return reject("optional header too small");
        get(opt + 16, entry);
        get(opt + 24, base);
        get(opt + 32, sectAlign);
        get(opt + 36, fileAlign);
        get(opt + 56, imageSize);
        get(opt + 60, headerSize);
        get(opt + 68, subsystem);
        get(opt + 70, dllChars);
        get(opt + 108, nDirs);
        std::printf("Magic 0x%x\nAddressOfEntryPoint 0x%x\nImageBase 0x%" PRIx64
                    "\nSectionAlignment %u\nFileAlignment %u\nSizeOfImage %u\nSizeOfHeaders %u\n"
                    "Subsystem 0x%x\nDllCharacteristics 0x%x\nNumberOfRvaAndSize %u\n",
                    magic, entry, base, sectAlign, fileAlign, imageSize, headerSize, subsystem,
                    dllChars, nDirs);
        // Data directory 5 is the base relocation table (8 bytes per directory entry).
        if (nDirs > 5 && 112u + 6u * 8u <= optSize) {
            get(opt + 112 + 5 * 8, relocRva);
            get(opt + 112 + 5 * 8 + 4, relocSize);
            std::printf("BaseRelocationTableRVA 0x%x\nBaseRelocationTableSize 0x%x\n", relocRva,
                        relocSize);
        }
    }

    // Section table: 40 bytes per section, right after the optional header.
    const std::uint64_t sect = opt + optSize;
    for (unsigned i = 0; i < nsect; ++i) {
        const std::uint64_t s = sect + 40ull * i;
        char name[9] = {};
        std::uint32_t vsize = 0, vaddr = 0, rawSize = 0, rawPtr = 0, relPtr = 0, flags = 0;
        std::uint16_t nrel = 0;
        if (!inFile(s, 40)) return reject("section table outside the file");
        std::memcpy(name, file.data() + s, 8);
        std::string longName = name;
        // Names longer than 8 bytes (object files only) are written as "/n": n is the decimal
        // offset of the real name in the COFF string table, which follows the symbol table
        // (18 bytes per symbol).
        if (name[0] == '/' && symPtr != 0) {
            const std::uint64_t strtab = symPtr + 18ull * nsym;
            const std::uint64_t at = strtab + std::strtoull(name + 1, nullptr, 10);
            if (!inFile(at, 1)) return reject("long section name outside the file");
            const auto* p = reinterpret_cast<const char*>(file.data() + at);
            const void* nul = std::memchr(p, 0, file.size() - at);
            if (nul == nullptr) return reject("unterminated long section name");
            longName.assign(p, static_cast<const char*>(nul));
        }
        get(s + 8, vsize);
        get(s + 12, vaddr);
        get(s + 16, rawSize);
        get(s + 20, rawPtr);
        get(s + 24, relPtr);
        get(s + 32, nrel);
        get(s + 36, flags);
        if (rawSize != 0 && !inFile(rawPtr, rawSize)) return reject("section data outside the file");
        std::printf("Section %u Name %s VirtualSize 0x%x VirtualAddress 0x%x RawDataSize %u "
                    "PointerToRawData 0x%x PointerToRelocations 0x%x RelocationCount %u "
                    "Characteristics 0x%x\n",
                    i + 1, longName.c_str(), vsize, vaddr, rawSize, rawPtr, relPtr, nrel, flags);
        // Base relocations: find the section that holds the table's RVA, then walk its blocks.
        if (relocSize != 0 && relocRva >= vaddr && relocRva - vaddr < rawSize) {
            std::uint64_t at = rawPtr + (relocRva - vaddr);
            const std::uint64_t end = at + relocSize;
            while (at + 8 <= end) {
                std::uint32_t page = 0, blockSize = 0;
                if (!get(at, page) || !get(at + 4, blockSize)) return reject("truncated block");
                if (blockSize < 8 || at + blockSize > end) return reject("bad block size");
                for (std::uint64_t e = at + 8; e + 2 <= at + blockSize; e += 2) {
                    std::uint16_t entry = 0;
                    get(e, entry);
                    const unsigned type = entry >> 12;   // high 4 bits: type
                    if (type == 0) continue;             // padding entry
                    std::printf("BaseReloc Type %u Address 0x%x\n", type, page + (entry & 0xfff));
                }
                at += blockSize;
            }
        }
    }
    return 0;
}
