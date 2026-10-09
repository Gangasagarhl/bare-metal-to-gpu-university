// elfinfo.cc - host test bench for elf64.hpp (the loader's ELF check, F3-15).
//   elfinfo <kernel.elf>          print what the loader would load, or why it would refuse
//   elfinfo --attack <kernel.elf> feed damaged copies of the file to the same check
// Built with AddressSanitizer and UBSan, so a read outside the buffer would stop the run.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include "elf64.hpp"

namespace {

using Bytes = std::vector<std::uint8_t>;

const char* check(const Bytes& b, elf::Image* img)
{
    return elf::check(b.data(), b.size(), img);
}

void put64(Bytes& b, std::size_t off, std::uint64_t v)
{
    for (int i = 0; i < 8; ++i) {
        b.at(off + static_cast<std::size_t>(i)) = static_cast<std::uint8_t>(v >> (8 * i));
    }
}

int show(const Bytes& b)
{
    elf::Image img;
    if (const char* why = check(b, &img)) {
        std::printf("rejected: %s\n", why);
        return 1;
    }
    std::printf("accepted: entry 0x%016llx, %d PT_LOAD segments, span 0x%llx bytes\n",
                static_cast<unsigned long long>(img.entry), img.count,
                static_cast<unsigned long long>(img.highest - img.lowest));
    for (int i = 0; i < img.count; ++i) {
        const elf::Segment& s = img.seg[i];
        std::printf("  LOAD vaddr 0x%016llx  file offset 0x%06llx  filesz 0x%06llx  memsz 0x%06llx  %c%c%c\n",
                    static_cast<unsigned long long>(s.vaddr), static_cast<unsigned long long>(s.offset),
                    static_cast<unsigned long long>(s.filesz), static_cast<unsigned long long>(s.memsz),
                    s.flags & elf::kFlagR ? 'R' : '-', s.flags & elf::kFlagW ? 'W' : '-',
                    s.flags & elf::kFlagX ? 'X' : '-');
    }
    return 0;
}

int attack(const Bytes& good)
{
    elf::Image img;
    struct Case {
        const char* name;
        Bytes data;
    };
    std::vector<Case> cases;
    cases.push_back({"unchanged file", good});
    cases.push_back({"truncated to 40 bytes", Bytes(good.begin(), good.begin() + 40)});
    cases.push_back({"truncated to 200 bytes", Bytes(good.begin(), good.begin() + 200)});
    Bytes b = good;
    b[0] = 'X';
    cases.push_back({"first byte changed (bad magic)", b});
    b = good;
    b[4] = 1;
    cases.push_back({"class byte says 32-bit", b});
    b = good;
    put64(b, 32, 0xfffffffffffff000ull);
    cases.push_back({"program header offset huge", b});
    b = good;
    put64(b, 64 + 56 + 16, elf::get(good.data() + 64 + 16, 8));
    cases.push_back({"second segment moved onto the first (overlap)", b});
    b = good;
    put64(b, 24, 0x400000);
    cases.push_back({"entry point outside every segment", b});
    b = good;
    put64(b, 64 + 8, good.size());
    cases.push_back({"first segment's file offset at end of file", b});
    for (const Case& c : cases) {
        const char* why = check(c.data, &img);
        std::printf("  %-48s -> %s\n", c.name, why ? why : "accepted");
    }
    // random damage: flip 1 to 8 bytes in the first 512 bytes (the headers), many times
    std::mt19937 rng(301);
    int accepted = 0, rejected = 0;
    const int runs = 200000;
    for (int i = 0; i < runs; ++i) {
        Bytes d = good;
        const int flips = 1 + static_cast<int>(rng() % 8);
        for (int k = 0; k < flips; ++k) {
            d[rng() % 512] = static_cast<std::uint8_t>(rng());
        }
        (check(d, &img) ? rejected : accepted) += 1;
    }
    std::printf("random damage: %d damaged copies, %d rejected, %d still accepted, no crash\n", runs,
                rejected, accepted);
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    const bool attack_mode = argc == 3 && std::string(argv[1]) == "--attack";
    if (argc != 2 && !attack_mode) {
        std::printf("usage: elfinfo [--attack] <kernel.elf>\n");
        return 2;
    }
    std::ifstream in(argv[argc - 1], std::ios::binary);
    const Bytes data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.empty()) {
        std::printf("cannot read %s\n", argv[argc - 1]);
        return 1;
    }
    return attack_mode ? attack(data) : show(data);
}
