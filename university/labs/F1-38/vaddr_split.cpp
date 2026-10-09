// Split 48-bit x86-64 virtual addresses into the four 9-bit table indices and the 12-bit
// page offset used by 4-level paging with 4 KiB pages. Input: one hexadecimal address per line.
// The last line splits the address of a local variable of this very run.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

void split(std::uint64_t va)
{
    const unsigned l4 = (va >> 39) & 0x1FF;    // bits 47..39: entry in the top-level table
    const unsigned l3 = (va >> 30) & 0x1FF;    // bits 38..30
    const unsigned l2 = (va >> 21) & 0x1FF;    // bits 29..21
    const unsigned l1 = (va >> 12) & 0x1FF;    // bits 20..12: entry in the last table
    const unsigned offset = va & 0xFFF;        // bits 11..0: byte inside the 4 KiB page
    std::printf("0x%012llx -> level-4 %3u, level-3 %3u, level-2 %3u, level-1 %3u, offset 0x%03x\n",
                static_cast<unsigned long long>(va), l4, l3, l2, l1, offset);
}

int main()
{
    std::string word;
    while (std::cin >> word) {
        split(std::stoull(word, nullptr, 16));
    }
    int local = 0;
    std::printf("a local variable of this run:\n");
    split(reinterpret_cast<std::uintptr_t>(&local));
    return 0;
}
