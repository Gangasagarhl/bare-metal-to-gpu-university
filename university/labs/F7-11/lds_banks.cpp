// F7-11 Listing 1: a bank model of the LDS. Words of 4 bytes are spread over `banks` banks
// (word w lives in bank w % banks). A wave's access is served `group` lanes at a time;
// within one group, a bank that must deliver d different words needs d passes.
// Both numbers are PARAMETERS: check them for your GPU in its ISA reference guide.
#include <algorithm>
#include <cstdio>
#include <initializer_list>
#include <map>
#include <set>
#include <string>
#include <vector>

// Passes needed for one wave-wide access; word[l] is the word address of lane l.
int passes(const std::vector<long>& word, int banks, int group)
{
    int total = 0;
    for (std::size_t first = 0; first < word.size(); first += static_cast<std::size_t>(group)) {
        std::map<long, std::set<long>> wordsInBank;          // bank -> distinct words asked for
        const std::size_t last = std::min(word.size(), first + static_cast<std::size_t>(group));
        for (std::size_t l = first; l < last; ++l) {
            wordsInBank[word[l] % banks].insert(word[l]);
        }
        int worst = 0;
        for (const auto& kv : wordsInBank) {
            worst = std::max(worst, static_cast<int>(kv.second.size()));
        }
        total += worst;                                       // the busiest bank decides
    }
    return total;
}

std::vector<long> strided(int lanes, long stride, long start)
{
    std::vector<long> w(static_cast<std::size_t>(lanes));
    for (int l = 0; l < lanes; ++l) {
        w[static_cast<std::size_t>(l)] = start + stride * l;
    }
    return w;
}

int main()
{
    const int lanes = 64;   // one wavefront on a CDNA target
    const int banks = 32;   // model value: see the chapter's unverified box
    std::printf("wave of %d lanes, 4-byte words; columns: banks / lanes served together\n", lanes);
    std::printf("%-44s %10s %10s %10s\n", "access pattern (word address of lane l)",
                "32 / 64", "32 / 32", "64 / 64");
    struct Case { std::string name; long stride; };
    for (const Case& c : {Case{"same word for all lanes (broadcast): 0*l", 0},
                          Case{"row of a tile: 1*l", 1},
                          Case{"every second word: 2*l", 2},
                          Case{"stride 4: 4*l", 4},
                          Case{"stride 16: 16*l", 16},
                          Case{"stride 32: 32*l", 32},
                          Case{"stride 33 (pitch 33): 33*l", 33},
                          Case{"column of tile[64][64]: 64*l", 64},
                          Case{"column of tile[64][65]: 65*l", 65}}) {
        const std::vector<long> w = strided(lanes, c.stride, 0);
        std::printf("%-44s %10d %10d %10d\n", c.name.c_str(), passes(w, banks, 64),
                    passes(w, banks, 32), passes(w, 2 * banks, 64));
    }
    {
        // tile[64][64] with an XOR swizzle: element (row, col) is stored at row * 64 + (col ^ row).
        // A column read (lane l reads row l of column c) touches word l * 64 + (c ^ l); here c = 5.
        std::vector<long> w(static_cast<std::size_t>(lanes));
        for (int l = 0; l < lanes; ++l) {
            w[static_cast<std::size_t>(l)] = 64L * l + (5 ^ l);
        }
        std::printf("%-44s %10d %10d %10d\n", "column 5 of tile[64][64], XOR swizzle", passes(w, banks, 64),
                    passes(w, banks, 32), passes(w, 2 * banks, 64));
    }
    std::printf("lower bound for 64 different words: 64 words / banks = 2, 2 and 1 passes\n");
    return 0;
}
