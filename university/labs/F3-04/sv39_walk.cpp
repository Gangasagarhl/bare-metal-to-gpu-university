// sv39_walk.cpp - the university's model of a three-level page-table walk in the style of
// RISC-V Sv39: 39-bit virtual addresses, 4 KiB pages, 512 entries per table.
// Simplified: no accessed/dirty bits, no large pages, no TLB.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using u64 = std::uint64_t;
constexpr u64 V = 1, R = 2, W = 4, X = 8, U = 16;  // permission bits of an entry

struct Pte {
    u64 flags = 0;  // 0 means "not valid"
    u64 ppn = 0;    // physical page number: of the next table, or of the final page
};
using Table = std::vector<Pte>;

std::map<u64, Table> memory;  // physical page number -> a page-table page
u64 nextFree = 0x80010;       // where our toy allocator hands out pages for tables
const u64 root = 0x80001;     // physical page number of the root table

Table& table(u64 ppn)
{
    auto it = memory.find(ppn);
    if (it == memory.end()) { it = memory.emplace(ppn, Table(512)).first; }
    return it->second;
}
u64 vpn(u64 va, int level) { return (va >> (12 + 9 * level)) & 0x1ff; }  // 9 bits per level

void map(u64 va, u64 pa, u64 flags)
{
    u64 ppn = root;
    for (int level = 2; level > 0; --level) {
        Pte& e = table(ppn)[vpn(va, level)];
        if (!(e.flags & V)) { e = Pte{V, nextFree++}; }  // a pointer to a next-level table
        ppn = e.ppn;
    }
    table(ppn)[vpn(va, 0)] = Pte{flags | V, pa >> 12};
}

void translate(u64 va, char access, char mode)
{
    std::printf("translate 0x%llx (%c, %s mode)\n", static_cast<unsigned long long>(va), access,
                mode == 'u' ? "user" : "kernel");
    std::printf(
        "  VPN[2]=%llu VPN[1]=%llu VPN[0]=%llu offset=0x%llx\n",
        static_cast<unsigned long long>(vpn(va, 2)), static_cast<unsigned long long>(vpn(va, 1)),
        static_cast<unsigned long long>(vpn(va, 0)), static_cast<unsigned long long>(va & 0xfff));
    u64 ppn = root;
    for (int level = 2; level >= 0; --level) {
        const Pte e = table(ppn)[vpn(va, level)];
        std::printf("  level %d: table at page 0x%llx, entry %llu -> ", level,
                    static_cast<unsigned long long>(ppn),
                    static_cast<unsigned long long>(vpn(va, level)));
        if (!(e.flags & V)) {
            std::printf("not valid => PAGE FAULT\n");
            return;
        }
        if (level > 0) {
            std::printf("next table at page 0x%llx\n", static_cast<unsigned long long>(e.ppn));
            ppn = e.ppn;
            continue;
        }
        std::printf("page 0x%llx flags %s%s%s%s\n", static_cast<unsigned long long>(e.ppn),
                    e.flags & R ? "R" : "-", e.flags & W ? "W" : "-", e.flags & X ? "X" : "-",
                    e.flags & U ? "U" : "-");
        const u64 need = access == 'r' ? R : access == 'w' ? W : X;
        if (!(e.flags & need)) {
            std::printf("  access not allowed => PAGE FAULT\n");
            return;
        }
        if (mode == 'u' && !(e.flags & U)) {
            std::printf("  user may not touch a kernel page => PAGE FAULT\n");
            return;
        }
        const u64 pa = (e.ppn << 12) | (va & 0xfff);
        std::printf("  physical address 0x%llx\n", static_cast<unsigned long long>(pa));
    }
}

int main()
{
    std::string cmd;
    while (std::cin >> cmd) {
        if (cmd == "map") {
            u64 va = 0, pa = 0;
            std::string perms;
            std::cin >> std::hex >> va >> pa >> std::dec >> perms;
            u64 f = 0;
            for (char c : perms) {
                f |= c == 'R' ? R : c == 'W' ? W : c == 'X' ? X : c == 'U' ? U : 0;
            }
            map(va, pa, f);
            std::printf("map 0x%llx -> 0x%llx %s\n", static_cast<unsigned long long>(va),
                        static_cast<unsigned long long>(pa), perms.c_str());
        } else if (cmd == "translate") {
            u64 va = 0;
            char access = 'r', mode = 'u';
            std::cin >> std::hex >> va >> std::dec >> access >> mode;
            translate(va, access, mode);
        }
    }
    std::printf("page-table pages in use: %zu\n", memory.size());
    return 0;
}
