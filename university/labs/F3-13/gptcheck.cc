// gptcheck.cc - milestone A2: verify a GPT disk image independently of the tools that made it.
// Checks the protective MBR, both GPT headers with their CRC32 values, the partition entries,
// and that the EFI System Partition (FAT32) holds \EFI\BOOT\BOOTX64.EFI.
// Field offsets were written from memory of the UEFI Specification ("GUID Partition Table (GPT)
// Disk Layout") and the Microsoft FAT32 specification; see the chapter's unverified box.
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

using Bytes = std::vector<std::uint8_t>;
constexpr std::size_t kSector = 512;

std::uint32_t u16(const Bytes& b, std::size_t o) { return b.at(o) | b.at(o + 1) << 8; }
std::uint32_t u32(const Bytes& b, std::size_t o) { return u16(b, o) | u16(b, o + 2) << 16; }
std::uint64_t u64(const Bytes& b, std::size_t o) { return u32(b, o) | static_cast<std::uint64_t>(u32(b, o + 4)) << 32; }

std::uint32_t crc32(const Bytes& b, std::size_t off, std::size_t len)  // reflected, poly 0xEDB88320
{
    std::uint32_t crc = 0xffffffff;
    for (std::size_t i = off; i < off + len; ++i) {
        crc ^= b.at(i);
        for (int k = 0; k < 8; ++k) {
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

std::string guid(const Bytes& b, std::size_t o)  // mixed-endian text form
{
    char s[40];
    std::snprintf(s, sizeof s, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", u32(b, o), u16(b, o + 4),
                  u16(b, o + 6), b[o + 8], b[o + 9], b[o + 10], b[o + 11], b[o + 12], b[o + 13], b[o + 14], b[o + 15]);
    return s;
}

int failures = 0;
void check(bool ok, const std::string& what)
{
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what.c_str());
    failures += ok ? 0 : 1;
}

struct Header {
    bool valid = false;
    std::uint64_t entries_lba = 0;
    std::uint32_t count = 0, entry_size = 0, entries_crc = 0;
};

Header read_header(const Bytes& d, std::uint64_t lba, std::uint64_t expect_other, const char* label)
{
    std::printf("%s GPT header at LBA %llu\n", label, static_cast<unsigned long long>(lba));
    const std::size_t o = lba * kSector;
    Header h;
    const bool sig = std::string(d.begin() + static_cast<long>(o), d.begin() + static_cast<long>(o + 8)) == "EFI PART";
    check(sig, "signature \"EFI PART\"");
    if (!sig) {
        return h;
    }
    const std::uint32_t size = u32(d, o + 12);
    Bytes copy(d.begin() + static_cast<long>(o), d.begin() + static_cast<long>(o + size));
    copy[16] = copy[17] = copy[18] = copy[19] = 0;  // the CRC is computed with its own field zeroed
    const std::uint32_t stored = u32(d, o + 16), computed = crc32(copy, 0, size);
    char msg[120];
    std::snprintf(msg, sizeof msg, "revision 0x%08x, header size %u", u32(d, o + 8), size);
    check(u32(d, o + 8) == 0x00010000 && size == 92, msg);
    std::snprintf(msg, sizeof msg, "header CRC32 stored 0x%08x, computed 0x%08x", stored, computed);
    check(stored == computed, msg);
    std::snprintf(msg, sizeof msg, "my LBA %llu, alternate LBA %llu", static_cast<unsigned long long>(u64(d, o + 24)),
                  static_cast<unsigned long long>(u64(d, o + 32)));
    check(u64(d, o + 24) == lba && u64(d, o + 32) == expect_other, msg);
    std::printf("  usable LBAs %llu..%llu, disk GUID %s\n", static_cast<unsigned long long>(u64(d, o + 40)),
                static_cast<unsigned long long>(u64(d, o + 48)), guid(d, o + 56).c_str());
    h.entries_lba = u64(d, o + 72);
    h.count = u32(d, o + 80);
    h.entry_size = u32(d, o + 84);
    h.entries_crc = u32(d, o + 88);
    const std::uint32_t entries = crc32(d, h.entries_lba * kSector, std::size_t{h.count} * h.entry_size);
    std::snprintf(msg, sizeof msg, "entries at LBA %llu: %u x %u bytes, CRC32 stored 0x%08x, computed 0x%08x",
                  static_cast<unsigned long long>(h.entries_lba), h.count, h.entry_size, h.entries_crc, entries);
    check(entries == h.entries_crc, msg);
    h.valid = stored == computed && entries == h.entries_crc;
    return h;
}

// FAT32: find a name (11 characters, 8.3 form) in the directory that starts at `cluster`.
std::uint32_t find(const Bytes& d, std::size_t part, std::uint32_t cluster, const std::string& name, std::uint32_t* size)
{
    const std::uint32_t bps = u16(d, part + 11), spc = d.at(part + 13), reserved = u16(d, part + 14);
    const std::uint32_t fats = d.at(part + 16), fat_size = u32(d, part + 36);
    const std::size_t fat = part + std::size_t{reserved} * bps;
    const std::size_t data = fat + std::size_t{fats} * fat_size * bps;
    while (cluster >= 2 && cluster < 0x0ffffff8) {
        const std::size_t dir = data + std::size_t{cluster - 2} * spc * bps;
        for (std::size_t e = dir; e < dir + std::size_t{spc} * bps; e += 32) {
            if (d.at(e) == 0) {
                return 0;                                  // end of directory
            }
            if (d.at(e + 11) == 0x0f || d.at(e) == 0xe5) {
                continue;                                  // long-name piece or deleted entry
            }
            if (std::string(d.begin() + static_cast<long>(e), d.begin() + static_cast<long>(e + 11)) == name) {
                *size = u32(d, e + 28);
                return u16(d, e + 20) << 16 | u16(d, e + 26);
            }
        }
        cluster = u32(d, fat + std::size_t{cluster} * 4) & 0x0fffffff;  // next cluster of the directory
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::printf("usage: gptcheck <disk image>\n");
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary | std::ios::ate);
    Bytes d(in ? static_cast<std::size_t>(in.tellg()) : 0);
    in.seekg(0);
    in.read(reinterpret_cast<char*>(d.data()), static_cast<std::streamsize>(d.size()));
    if (d.size() < 64 * kSector) {
        std::printf("cannot read %s or it is too small\n", argv[1]);
        return 1;
    }
    const std::uint64_t last = d.size() / kSector - 1;
    std::printf("image %s: %zu bytes, %llu sectors of %zu bytes\n", argv[1], d.size(),
                static_cast<unsigned long long>(last + 1), kSector);

    std::printf("protective MBR at LBA 0\n");
    check(d[510] == 0x55 && d[511] == 0xaa, "boot signature 0x55 0xAA at offsets 510-511");
    char msg[120];
    std::snprintf(msg, sizeof msg, "partition entry 1: type 0x%02x, starting LBA %u, %u sectors", d[446 + 4],
                  u32(d, 446 + 8), u32(d, 446 + 12));
    check(d[446 + 4] == 0xee && u32(d, 446 + 8) == 1, msg);

    const Header primary = read_header(d, 1, last, "primary");
    const Header backup = read_header(d, last, 1, "backup");
    const Header& use = primary.valid ? primary : backup;
    std::printf("partition entries (from the %s header)\n", primary.valid ? "primary" : "backup");
    if (!use.valid) {
        std::printf("  no valid header: cannot list partitions\n");
        return 1;
    }
    check(primary.entries_crc == backup.entries_crc, "primary and backup entry arrays have the same CRC32");
    const std::string esp_type = "c12a7328-f81f-11d2-ba4b-00a0c93ec93b";
    std::size_t esp_lba = 0;
    for (std::uint32_t i = 0; i < use.count; ++i) {
        const std::size_t e = use.entries_lba * kSector + std::size_t{i} * use.entry_size;
        if (guid(d, e) == "00000000-0000-0000-0000-000000000000") {
            continue;                                      // unused entry
        }
        std::string name;
        for (std::size_t c = e + 56; c < e + 128 && d[c] != 0; c += 2) {
            name += static_cast<char>(d[c]);               // UTF-16LE; ASCII names only here
        }
        std::printf("  %u: type %s%s\n     unique %s, LBA %llu..%llu, name \"%s\"\n", i + 1, guid(d, e).c_str(),
                    guid(d, e) == esp_type ? " (EFI System Partition)" : "", guid(d, e + 16).c_str(),
                    static_cast<unsigned long long>(u64(d, e + 32)), static_cast<unsigned long long>(u64(d, e + 40)),
                    name.c_str());
        if (guid(d, e) == esp_type && esp_lba == 0) {
            esp_lba = u64(d, e + 32);
        }
    }
    check(esp_lba != 0, "an EFI System Partition exists");
    if (esp_lba != 0) {
        const std::size_t part = esp_lba * kSector;
        std::printf("EFI System Partition file system\n");
        check(std::string(d.begin() + static_cast<long>(part + 82), d.begin() + static_cast<long>(part + 90)) == "FAT32   ",
              "boot sector says \"FAT32\"");
        std::uint32_t size = 0;
        std::uint32_t c = find(d, part, u32(d, part + 44), "EFI        ", &size);
        c = c ? find(d, part, c, "BOOT       ", &size) : 0;
        c = c ? find(d, part, c, "BOOTX64 EFI", &size) : 0;
        std::snprintf(msg, sizeof msg, "\\EFI\\BOOT\\BOOTX64.EFI present (%u bytes, first cluster %u)", size, c);
        check(c != 0, msg);
    }
    std::printf("result: %s (%d failed checks)\n", failures == 0 ? "PASS" : "FAIL", failures);
    return failures == 0 ? 0 : 1;
}
