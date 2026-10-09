// reset_bytes.cpp - where does the first instruction come from?
// Reads the OVMF firmware image that QEMU maps as flash just below 4 GiB, and prints the
// 16 bytes that end up at physical addresses 0xFFFFFFF0..0xFFFFFFFF (the reset vector area).
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    // default: the image the lab boots; the forensic lab passes another file name
    const std::string path = argc > 1 ? argv[1] : "/usr/share/OVMF/OVMF_CODE_4M.fd";
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::printf("cannot open %s (is the ovmf package installed?)\n", path.c_str());
        return 1;
    }
    const std::vector<unsigned char> image((std::istreambuf_iterator<char>(in)),
                                           std::istreambuf_iterator<char>());
    const std::uint64_t four_gib = std::uint64_t{1} << 32;
    const std::uint64_t size = image.size();
    const std::uint64_t base = four_gib - size;  // the image's last byte sits at 4 GiB - 1

    std::printf("firmware image: %s\n", path.c_str());
    std::printf("size: %llu bytes = %llu KiB\n", static_cast<unsigned long long>(size),
                static_cast<unsigned long long>(size / 1024));
    std::printf("if its last byte is at 0xffffffff, it starts at 0x%08llx\n",
                static_cast<unsigned long long>(base));
    std::printf("the last 16 bytes of the file, as the CPU sees them at physical addresses:\n");
    for (std::uint64_t addr = four_gib - 16; addr < four_gib; ++addr) {
        if (addr % 8 == 0) {
            std::printf("%s0x%08llx:", addr == four_gib - 16 ? "" : "\n",
                        static_cast<unsigned long long>(addr));
        }
        std::printf(" %02x", image[addr - base]);
    }
    std::printf("\n");
    return 0;
}
