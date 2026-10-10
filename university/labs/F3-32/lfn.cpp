// lfn.cpp - F3-32: short-name generation and the long-name checksum, worked by a program.
#include <cstdio>
#include <string>
#include "fat_names.h"

int main()
{
    const char* names[] = {"README.TXT", "readme.txt", "Read me first.txt", "Specification notes.txt",
                           "archive.tar.gz", "a+b=c.txt", "Quarterly Report Draft.txt"};
    std::printf("%-28s %-14s %-6s %-8s %s\n", "long name", "short name", "lossy", "recased", "checksum");
    for (const char* n : names) {
        bool lossy = false, recased = false;
        ShortName sn = short_basis(n, lossy, recased);
        if (lossy) sn = with_tail(sn, 1);              // first free tail; the driver checks uniqueness
        std::printf("%-28s %-14s %-6s %-8s 0x%02X\n", n, short_to_string(sn).c_str(), lossy ? "yes" : "no",
                    recased ? "yes" : "no", lfn_checksum(sn));
    }
    // The checksum step by step for one name, to compare with a hand calculation.
    bool lossy = false, recased = false;
    ShortName sn = with_tail(short_basis("Read me first.txt", lossy, recased), 1);
    std::uint8_t sum = 0;
    std::printf("\nchecksum of \"%s\" stored as \"", short_to_string(sn).c_str());
    for (auto c : sn) std::printf("%c", c);
    std::printf("\":\n");
    for (int i = 0; i < 11; ++i) {
        std::uint8_t rot = static_cast<std::uint8_t>(((sum & 1) ? 0x80 : 0) + (sum >> 1));
        std::uint8_t next = static_cast<std::uint8_t>(rot + sn[i]);
        std::printf("  byte %2d '%c' 0x%02X: rotate 0x%02X -> 0x%02X, add -> 0x%02X\n", i, sn[i], sn[i], sum, rot, next);
        sum = next;
    }
    return 0;
}
