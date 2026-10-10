// F7-20 Listing 2: a tiny scanner for warp-size assumptions in GPU source (reads stdin).
// It flags patterns, not bugs: every hit must be read by a person.
#include <cstdio>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

struct Rule { std::regex re; const char* why; };

int main()
{
    const std::vector<Rule> rules = {
        {std::regex(R"((%|/|&)\s*(32|31)\b)"), "lane or warp index computed with a literal 32/31"},
        {std::regex(R"(\b0x[fF]{8}\b)"), "32-bit full lane mask"},
        {std::regex(R"(\bunsigned( int)?\s+\w+\s*=\s*__ballot)"), "ballot stored in 32 bits"},
        {std::regex(R"(\b(int|unsigned)\s+\w+\s*=\s*16\s*;.*>\s*0)"), "shuffle tree starting at 16 lanes"},
        {std::regex(R"(\bWARP_SIZE\b|\bwarp_size\s*=\s*32)"), "hard-coded warp size constant"},
        {std::regex(R"(<<<[^>]*,\s*32\s*>>>)"), "launch with 32 threads per block"},
        {std::regex(R"(__shfl_xor\([^,]+,\s*16\))"), "shuffle distance 16 (half of a 32-lane warp)"},
    };
    std::string line;
    int lineNo = 0;
    int hits = 0;
    while (std::getline(std::cin, line)) {
        ++lineNo;
        for (const Rule& r : rules) {
            if (std::regex_search(line, r.re)) {
                std::printf("line %3d: %-48s | %s\n", lineNo, r.why, line.c_str());
                ++hits;
                break;
            }
        }
    }
    std::printf("%d lines flagged\n", hits);
    return 0;
}
