// HW201 answer keys: every number in the final paper's model answers is recomputed here
// (guide 11.4: numbers in a key come from a real run). Build with the course flags:
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined exam_checks.cpp
// Sections follow the question numbers of university/chapters/HW201/EXAMS.html.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

// ---- F2: NOR SR latch with a delay of 1 time unit per gate (as sr_latch.v, F1-16)
static void srLatch() {
    std::printf("F2: NOR SR latch, 1 unit per gate; start Q=0 Qn=1; S=1 at t=10, S=0 at t=14, R=1 at t=20, R=0 at t=24\n");
    int q = 0, qn = 1;
    for (int t = 9; t <= 27; ++t) {
        const int s = (t >= 10 && t < 14) ? 1 : 0;
        const int r = (t >= 20 && t < 24) ? 1 : 0;
        // outputs at time t+1 follow inputs at time t (one unit of delay)
        const int nq = (r == 0 && qn == 0) ? 1 : 0;
        const int nqn = (s == 0 && q == 0) ? 1 : 0;
        std::printf("  t=%2d  S=%d R=%d | Q=%d Qn=%d\n", t, s, r, q, qn);
        q = nq;
        qn = nqn;
    }
}

// ---- F4: a 4-bit shift register (q <= {q[2:0], serial_in}) and a 5-bit counter
static void shiftAndCount() {
    unsigned q = 0;
    const std::vector<int> bits = {1, 0, 0, 1, 1};
    std::printf("F4a: shift register from 0000, bits 1,0,0,1,1:");
    for (int b : bits) {
        q = ((q << 1) | static_cast<unsigned>(b)) & 0xFu;
        std::printf(" %u%u%u%u", (q >> 3) & 1u, (q >> 2) & 1u, (q >> 1) & 1u, q & 1u);
    }
    std::printf("\n");
    unsigned c = 30;
    std::printf("F4b: 5-bit counter from 30, enabled, 3 edges:");
    for (int i = 0; i < 3; ++i) {
        c = (c + 1) & 0x1Fu;
        std::printf(" %u", c);
    }
    std::printf("\n");
}

// ---- F7: clock arithmetic (exercise values)
static void clockMath() {
    const double clkToQ = 1.2, logic = 5.3, setup = 0.8;
    const double need = clkToQ + logic + setup;
    std::printf("F7a: 125 MHz -> period %.3f ns; 2.5 ns -> %.1f MHz\n", 1000.0 / 125.0, 1000.0 / 2.5);
    std::printf("F7b: path needs %.1f ns -> fmax %.1f MHz\n", need, 1000.0 / need);
    for (double f : {125.0, 150.0}) {
        const double period = 1000.0 / f;
        std::printf("F7c: at %.0f MHz, T = %.3f ns, slack = %+.3f ns (%s)\n", f, period, period - need,
                    period - need >= 0 ? "works" : "fails");
    }
    std::printf("F7d: 125 MHz clock, a pulse every 2 us: %.0f cycles -> count 0 to %.0f\n",
                2000.0 / 8.0, 2000.0 / 8.0 - 1);
}

// ---- F9: the 101 detector of F1-19 on a new bit string
static void seq101() {
    enum { S0, S1, S10, S101 };
    int state = S0;
    const std::vector<int> bits = {1, 0, 1, 1, 0, 1, 0, 1};
    std::printf("F9: 101 detector on 1,0,1,1,0,1,0,1; found after bits:");
    for (std::size_t i = 0; i < bits.size(); ++i) {
        const int in = bits[i];
        switch (state) {
        case S0: state = in ? S1 : S0; break;
        case S1: state = in ? S1 : S10; break;
        case S10: state = in ? S101 : S0; break;
        default: state = in ? S1 : S10; break;
        }
        if (state == S101) std::printf(" %zu", i + 1);
    }
    std::printf("\n");
}

// ---- F11: binary and one-hot encodings
static void encodings() {
    for (int n : {11, 6, 7}) {
        int bits = 0;
        while ((1 << bits) < n) ++bits;
        std::printf("F11: %d states -> binary %d flip-flops (%d codes, %d illegal), one-hot %d\n", n, bits,
                    1 << bits, (1 << bits) - n, n);
    }
}

// ---- F12: the DRAM model of F1-20 with keep = 0.92 per tick, threshold 0.5
static void dram() {
    const double keep = 0.92, threshold = 0.5;
    int lastSafe = 0;
    for (int k = 1; k <= 40; ++k) {
        if (std::pow(keep, k) > threshold) lastSafe = k;
    }
    std::printf("F12a: keep 0.92: 0.92^%d = %.3f, 0.92^%d = %.3f -> longest safe gap %d ticks\n", lastSafe,
                std::pow(keep, lastSafe), lastSafe + 1, std::pow(keep, lastSafe + 1), lastSafe);
    std::printf("F12b: round-robin over 8 rows (gap 8): %s; over 16 rows (gap 16): %s\n",
                8 <= lastSafe ? "safe" : "rows lost", 16 <= lastSafe ? "safe" : "rows lost");
    std::printf("F12c: a 1 after 20 ticks without refresh holds %.3f (reads as %d)\n", std::pow(keep, 20),
                std::pow(keep, 20) > threshold ? 1 : 0);
    std::printf("F13c: 12 address bits, 8-bit words: %d words, %d bits\n", 1 << 12, (1 << 12) * 8);
}

// ---- F16: 4-input LUT configurations, numbered as lut4.cpp (a = bit 0, b = 1, c = 2, d = 3)
static std::uint16_t configure(const std::function<int(int, int, int, int)>& fn) {
    std::uint16_t config = 0;
    for (int address = 0; address < 16; ++address) {
        const int a = address & 1, b = (address >> 1) & 1, c = (address >> 2) & 1, d = (address >> 3) & 1;
        if (fn(a, b, c, d)) config = static_cast<std::uint16_t>(config | (1u << address));
    }
    return config;
}
static void luts() {
    std::printf("F16a: XOR of a and b (c, d unused): config = 0x%04X\n",
                configure([](int a, int b, int, int) { return a ^ b; }));
    std::printf("F16b: NOR of all four inputs: config = 0x%04X\n",
                configure([](int a, int b, int c, int d) { return !(a | b | c | d); }));
    std::printf("F16c: bit 2 of count + 1 (count = d c b a): config = 0x%04X\n",
                configure([](int a, int b, int c, int d) {
                    const int count = (d << 3) | (c << 2) | (b << 1) | a;
                    return ((count + 1) >> 2) & 1;
                }));
    std::printf("F16d: a 5-input LUT needs %d configuration bits; a 6-input LUT %d\n", 1 << 5, 1 << 6);
}

// ---- F15: blocking and non-blocking swap, 8-bit program counter trace (F1-21)
static void assignments() {
    {
        int a = 3, b = 9;
        const int oldA = a, oldB = b;
        a = oldB; b = oldA;   // non-blocking: both right-hand sides read first
        std::printf("F15a: a <= b; b <= a; from a=3 b=9 gives a=%d b=%d\n", a, b);
    }
    {
        int a = 3, b = 9;
        a = b; b = a;         // blocking: the second line sees the new a
        std::printf("F15a: a = b; b = a; from a=3 b=9 gives a=%d b=%d\n", a, b);
    }
    unsigned pc = 0;
    struct Step { int reset, load, inc; unsigned jump; };
    const std::vector<Step> script = {{1, 0, 0, 0}, {0, 1, 1, 250}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0},
                                      {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {1, 1, 1, 7}};
    std::printf("F15b: program_counter (WIDTH 8): reset; load 250 with inc; inc x5; hold; reset+load+inc ->");
    for (const Step& s : script) {
        if (s.reset) pc = 0;
        else if (s.load) pc = s.jump;
        else if (s.inc) pc = (pc + 1) & 0xFFu;
        std::printf(" %u", pc);
    }
    std::printf("\n");
}

// ---- F17: board_top.v flip-flops (F1-22): ticks is 32 bits; bits needed for DIV - 1
static void boardBits() {
    for (unsigned div : {4u, 50000000u}) {
        int bits = 0;
        while ((1ull << bits) < div) ++bits;
        std::printf("F17: DIV = %u -> ticks counts 0..%u -> %d bits (instead of 32)\n", div, div - 1, bits);
    }
}

// ---- Practical (P): the phase lengths and the timeline of the reference run
static void practical() {
    const int wash = 6, rinse = 4, dry = 3;
    std::printf("P: start sampled at edge 3 -> wash cycles 3..%d, rinse %d..%d, dry %d..%d, done from %d\n",
                3 + wash - 1, 3 + wash, 3 + wash + rinse - 1, 3 + wash + rinse, 3 + wash + rinse + dry - 1,
                3 + wash + rinse + dry);
    int bits = 0;
    while ((1 << bits) < wash) ++bits;
    std::printf("P: timer counts 0..%d -> %d bits; 6 states -> 3 bits; resume register 3 bits -> %d flip-flops\n",
                wash - 1, bits, 3 + bits + 3);
}

int main() {
    std::printf("HW201 exam key checks (all values are exercise values)\n");
    srLatch();
    shiftAndCount();
    clockMath();
    seq101();
    encodings();
    dram();
    luts();
    assignments();
    boardBits();
    practical();
    return 0;
}
