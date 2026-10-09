// F1-57 Listing 1: a SIMT model. One instruction stream, many lanes, an active mask.
// Program executed by every lane (lane = thread):
//     if (v is odd) { v = 3 * v + 1; }   // path A: 2 instructions
//     else          { v = v / 2;     }   // path B: 1 instruction
//     out = v;                           // after reconvergence: 1 instruction
// The model issues each path once for the whole group, with the lanes that do not
// take that path switched off in the mask. It counts issue slots and useful lane-slots.
#include <cstdio>
#include <string>
#include <vector>

struct Cost
{
    int issued = 0;     // instructions issued for the whole group
    long useful = 0;    // sum over issued instructions of active lanes
};

std::string maskText(const std::vector<bool>& m)
{
    std::string s;
    for (bool b : m) { s += b ? '#' : '.'; }
    return s;
}

Cost runGroup(std::vector<int> v, bool show)
{
    const std::size_t width = v.size();
    Cost c;
    std::vector<bool> odd(width), even(width);
    for (std::size_t l = 0; l < width; ++l) { odd[l] = (v[l] % 2) != 0; even[l] = !odd[l]; }
    auto issue = [&](const char* what, const std::vector<bool>& mask, int instructions) {
        int active = 0;
        for (bool b : mask) { active += b ? 1 : 0; }
        if (active == 0) {
            if (show) { std::printf("  %-22s %s  skipped (no lane active)\n", what, maskText(mask).c_str()); }
            return;
        }
        c.issued += instructions;
        c.useful += static_cast<long>(instructions) * active;
        if (show) { std::printf("  %-22s %s  %d instr x %d lanes\n", what, maskText(mask).c_str(), instructions, active); }
    };
    issue("path A (v = 3v + 1)", odd, 2);
    issue("path B (v = v / 2)", even, 1);
    for (std::size_t l = 0; l < width; ++l) { v[l] = odd[l] ? 3 * v[l] + 1 : v[l] / 2; }
    issue("reconverged (out = v)", std::vector<bool>(width, true), 1);
    return c;
}

void report(const char* name, int width, int (*value)(int lane))
{
    std::vector<int> v(static_cast<std::size_t>(width));
    for (int l = 0; l < width; ++l) { v[static_cast<std::size_t>(l)] = value(l); }
    Cost c = runGroup(v, width <= 8);
    double eff = 100.0 * static_cast<double>(c.useful) / (static_cast<double>(c.issued) * width);
    std::printf("%-26s width %2d: issued %d, useful lane-slots %3ld of %3ld, SIMD efficiency %5.1f %%\n",
                name, width, c.issued, c.useful, static_cast<long>(c.issued) * width, eff);
}

int allEven(int lane) { return 2 * lane; }
int alternate(int lane) { return lane; }
int firstLaneOdd(int lane) { return lane == 0 ? 1 : 2 * lane; }
int halves32(int lane) { return lane < 32 ? 2 * lane : 2 * lane + 1; }

int main()
{
    std::printf("Width 8, drawn lane by lane (# = lane active, . = lane masked off):\n");
    report("all even", 8, allEven);
    report("alternating odd/even", 8, alternate);
    report("only lane 0 odd", 8, firstLaneOdd);
    std::printf("\nThe same patterns at the widths seen in the toolchain evidence (32 and 64):\n");
    for (int w : {32, 64}) {
        report("all even", w, allEven);
        report("alternating odd/even", w, alternate);
        report("only lane 0 odd", w, firstLaneOdd);
        report("lanes 0-31 even, rest odd", w, halves32);
    }
    return 0;
}
