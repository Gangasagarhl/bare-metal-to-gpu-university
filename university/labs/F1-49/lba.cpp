// lba.cpp - F1-49 Listing 2: a disk is an array of numbered sectors.
// Builds a small disk image file, writes one sector by its logical block address (LBA),
// reads it back, and works out which larger "physical sector" holds it.
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int main()
{
    const std::size_t sectorSize = 512;   // logical sector size used by this image
    const std::size_t sectors = 64;
    const std::string path = "disk.img";

    {   // create an image of 64 zeroed sectors
        std::ofstream img(path, std::ios::binary);
        const std::vector<char> zeros(sectorSize * sectors, 0);
        img.write(zeros.data(), static_cast<std::streamsize>(zeros.size()));
    }

    const std::size_t lba = 5;
    const std::size_t offset = lba * sectorSize;
    {   // write one sector in place
        std::fstream img(path, std::ios::binary | std::ios::in | std::ios::out);
        std::array<char, 512> sector{};
        const std::string msg = "hello from LBA 5";
        msg.copy(sector.data(), msg.size());
        img.seekp(static_cast<std::streamoff>(offset));
        img.write(sector.data(), static_cast<std::streamsize>(sector.size()));
    }

    std::array<char, 512> back{};
    {
        std::ifstream img(path, std::ios::binary);
        img.seekg(static_cast<std::streamoff>(offset));
        img.read(back.data(), static_cast<std::streamsize>(back.size()));
    }
    std::printf("image size     : %zu bytes = %zu sectors of %zu bytes\n",
                std::filesystem::file_size(path), sectors, sectorSize);
    std::printf("LBA %zu starts at byte offset %zu * %zu = %zu\n", lba, lba, sectorSize, offset);
    std::printf("read back      : \"%s\"\n", back.data());

    // A device that reports 512-byte logical and 4096-byte physical sectors:
    const std::size_t physical = 4096;
    const std::size_t perPhysical = physical / sectorSize;
    std::printf("logical sectors per physical sector: %zu / %zu = %zu\n",
                physical, sectorSize, perPhysical);
    for (std::size_t l : {std::size_t{5}, std::size_t{8}, std::size_t{15}, std::size_t{16}}) {
        std::printf("LBA %2zu -> physical sector %zu, slot %zu of %zu\n",
                    l, l / perPhysical, l % perPhysical, perPhysical);
    }
    std::filesystem::remove(path);
    return 0;
}
