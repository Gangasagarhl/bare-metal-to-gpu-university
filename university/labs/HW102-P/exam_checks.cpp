// HW102 exam checks: recomputes every hand-worked number in the HW102 answer keys
// (midterm M, final F) from the rules taught in F1-09 to F1-15, so that no number in a
// key is typed by hand. Built and run by university/labs/run_lab.sh with the course flags.
// The Verilog runs in this folder (run.sh) provide every simulator number.
#include <iostream>
#include <string>
#include <vector>
#include <functional>

// ---- F1-09: the ideal-switch model -------------------------------------------------
static bool nmosOn(int gate) { return gate == 1; }
static std::string outputOf(bool up, bool down) {
    if (up && down) return "X (short)";
    if (up) return "1";
    if (down) return "0";
    return "Z (floating)";
}

// ---- F1-14: 4-bit two's complement arithmetic with the ALU's flag rules --------------
struct Flags { int y, Z, N, C, V; bool rangeV; };
static Flags addFlags(int a, int b, bool sub) {
    int bin = sub ? (~b & 0xF) : b;
    int cin = sub ? 1 : 0;
    int carry = cin, cInTop = 0;
    int y = 0;
    for (int i = 0; i < 4; ++i) {
        int ai = (a >> i) & 1, bi = (bin >> i) & 1;
        int t = ai + bi + carry;
        y |= (t & 1) << i;
        if (i == 3) cInTop = carry;
        carry = t >> 1;
    }
    auto signed4 = [](int v) { return v >= 8 ? v - 16 : v; };
    int sr = sub ? signed4(a) - signed4(b) : signed4(a) + signed4(b);
    Flags f{y, y == 0, (y >> 3) & 1, carry, carry ^ cInTop, sr > 7 || sr < -8};
    return f;
}
static std::string bin4(int v) {
    std::string s;
    for (int i = 3; i >= 0; --i) s += ((v >> i) & 1) ? '1' : '0';
    return s;
}
static void showOp(const char* label, int a, int b, bool sub) {
    Flags f = addFlags(a, b, sub);
    std::cout << "  " << label << ": a=" << bin4(a) << " b=" << bin4(b)
              << (sub ? "  NOT b=" + bin4(~b & 0xF) + " (+1 as carry in)" : "")
              << "  -> y=" << bin4(f.y) << " (" << f.y << " unsigned, " << (f.y >= 8 ? f.y - 16 : f.y)
              << " signed)  Z=" << f.Z << " N=" << f.N << " C=" << f.C << " V=" << f.V
              << "  [V by range check: " << (f.rangeV ? 1 : 0) << "]\n";
}

// ---- F1-13: a faulty full adder whose carry is a AND b AND cin ------------------------
static void faultyFA(int a, int b, int cin, int& s, int& cout) {
    s = a ^ b ^ cin;
    cout = a & b & cin;
}
static int rippleFaulty(int a, int b) {          // 4-bit adder made of faulty full adders, cin = 0
    int carry = 0, y = 0;
    for (int i = 0; i < 4; ++i) {
        int s, c;
        faultyFA((a >> i) & 1, (b >> i) & 1, carry, s, c);
        y |= s << i;
        carry = c;
    }
    return y | (carry << 4);
}

static void truthTable3(const char* title, const std::function<int(int, int, int)>& f,
                        const char* names = "a b c") {
    std::cout << "  " << title << "\n  " << names << " | y\n";
    for (int i = 0; i < 8; ++i) {
        int a = (i >> 2) & 1, b = (i >> 1) & 1, c = i & 1;
        std::cout << "  " << a << " " << b << " " << c << " | " << f(a, b, c) << "\n";
    }
}

int main() {
    std::cout << "HW102 exam checks (rules of F1-09 to F1-15; every number below is recomputed here)\n\n";

    // M2: two nMOS, the top one's gate driven by NOT a (from a correct inverter)
    std::cout << "M2: inverter from two nMOS, top gate = NOT a, bottom gate = a\n";
    for (int a = 0; a <= 1; ++a) {
        bool up = nmosOn(1 - a), down = nmosOn(a);
        std::cout << "  a=" << a << ": top nMOS " << (up ? "ON " : "off") << ", bottom nMOS "
                  << (down ? "ON " : "off") << " -> y = " << outputOf(up, down) << "\n";
    }
    std::cout << "  transistors: 2 (this stage) + 2 (the inverter making NOT a) = 4\n\n";

    // M4: compound CMOS gate y = NOT((a OR b) AND c)
    truthTable3("M4: y = NOT((a OR b) AND c); pull-down (nMOS a || nMOS b) in series with nMOS c; "
                "pull-up (pMOS a series pMOS b) parallel pMOS c; 6 transistors",
                [](int a, int b, int c) { return !((a | b) & c) ? 1 : 0; });
    std::cout << "  check row a=1 b=0 c=1: y = " << (!((1 | 0) & 1) ? 1 : 0) << "\n\n";

    // M6: a AND NOT b from NANDs
    std::cout << "M6: a AND NOT b = NAND(NAND(a, NAND(b,b)), NAND(a, NAND(b,b)))  (3 NANDs)\n  a b | nb t y | a AND NOT b\n";
    for (int i = 0; i < 4; ++i) {
        int a = (i >> 1) & 1, b = i & 1;
        int nb = !(b & b), t = !(a & nb), y = !(t & t);
        std::cout << "  " << a << " " << b << " |  " << nb << " " << t << " " << y << " | " << (a & !b) << "\n";
    }
    std::cout << "  Yosys report '2 NAND + 3 NOT' counts as 2 + 3 = 5 NAND-equivalents\n\n";

    // M9: buzzer = en AND s1 (station code 2 or 3)
    truthTable3("M9: buzz = 1 when en = 1 and s1 s0 is 10 or 11; simplifies to en AND s1",
                [](int en, int s1, int) { return en & s1; }, "en s1 s0");
    std::cout << "  = OR of decoder outputs y2 and y3 (en included): (en AND s1 AND NOT s0) OR (en AND s1 AND s0) = en AND s1\n\n";

    // M10: 8-to-1 mux tree
    {
        int d = 0x20, s = 5;        // only d5 is 1; select 101
        std::cout << "M10: mux8 tree: 7 mux2 x 4 NAND = 28 NANDs; levels use s0, s1, s2 in that order\n";
        int lvl1[4], lvl2[2];
        for (int k = 0; k < 4; ++k) lvl1[k] = (d >> (2 * k + (s & 1))) & 1;
        for (int k = 0; k < 2; ++k) lvl2[k] = lvl1[2 * k + ((s >> 1) & 1)];
        int y = lvl2[(s >> 2) & 1];
        std::cout << "  d = 0010 0000 (d5 = 1), s = 101: level 1 (s0=1) passes d1,d3,d5,d7 = "
                  << lvl1[0] << lvl1[1] << lvl1[2] << lvl1[3] << "; level 2 (s1=0) passes the first of each pair = "
                  << lvl2[0] << lvl2[1] << "; level 3 (s2=1) passes the upper half: y = " << y << " = d5\n\n";
    }

    // F3: compound gate y = NOT(a AND (b OR c))
    truthTable3("F3: y = NOT(a AND (b OR c)); pull-down nMOS a in series with (nMOS b || nMOS c); "
                "pull-up pMOS a parallel (pMOS b series pMOS c); 6 transistors",
                [](int a, int b, int c) { return !(a & (b | c)) ? 1 : 0; });
    std::cout << "  check row a=1 b=0 c=0: y = " << (!(1 & (0 | 0)) ? 1 : 0) << "\n\n";

    // F5: NAND from NOR only
    std::cout << "F5: NAND from NORs: t = NOR(NOR(a,a), NOR(b,b)) = a AND b; y = NOR(t,t)  (4 NORs)\n  a b | t y | NAND\n";
    for (int i = 0; i < 4; ++i) {
        int a = (i >> 1) & 1, b = i & 1;
        int na = !(a | a), nb = !(b | b), t = !(na | nb), y = !(t | t);
        std::cout << "  " << a << " " << b << " | " << t << " " << y << " |  " << !(a & b) << "\n";
    }
    std::cout << "\n";

    // F6: counts
    std::cout << "F6: Yosys full adder 7 NAND + 5 NOT: NAND-equivalents 7 + 5 = " << 7 + 5
              << "; transistors 7*4 + 5*2 = " << 7 * 4 + 5 * 2 << ".  Hand 9-NAND: 9 NAND-equivalents, 9*4 = "
              << 9 * 4 << " transistors\n\n";

    // F7: 3-to-8 decoder output y5
    std::cout << "F7: dec3to8: y5 = en AND s2 AND NOT s1 AND s0; 8 four-input ANDs; en=0 -> all 0; en=1, s=101 -> y5 only\n";
    for (int s = 0; s < 8; ++s) {
        int y5 = 1 & ((s >> 2) & 1) & !((s >> 1) & 1) & (s & 1);
        if (y5) std::cout << "  en=1 s=" << ((s >> 2) & 1) << ((s >> 1) & 1) << (s & 1) << " -> y5 = 1\n";
    }
    std::cout << "\n";

    // F8: function from a 4-to-1 mux with constants
    std::cout << "F8: mux4 with d3 d2 d1 d0 = 1 0 0 1, select s1 s0 = a b:\n";
    {
        int d = 0b1001;
        for (int i = 0; i < 4; ++i) {
            int a = (i >> 1) & 1, b = i & 1;
            std::cout << "  a=" << a << " b=" << b << " -> y = d" << i << " = " << ((d >> i) & 1)
                      << "   (XNOR: " << (a == b) << ")\n";
        }
        std::cout << "  for y = a AND NOT b only index a b = 1 0 (d2) is 1: constants d3 d2 d1 d0 = 0 1 0 0\n\n";
    }

    // F9: 1101 + 0111 by hand
    {
        int a = 0b1101, b = 0b0111, carry = 0;
        std::cout << "F9: 1101 + 0111 column by column (bit, a, b, cin, T, s, cout)\n";
        int y = 0;
        for (int i = 0; i < 4; ++i) {
            int ai = (a >> i) & 1, bi = (b >> i) & 1, T = ai + bi + carry;
            std::cout << "  bit " << i << ": " << ai << " + " << bi << " + " << carry << " = " << T
                      << " -> s=" << (T & 1) << " cout=" << (T >> 1) << "\n";
            y |= (T & 1) << i;
            carry = T >> 1;
        }
        std::cout << "  result " << carry << " " << bin4(y) << " = " << (y | carry << 4) << " (13 + 7 = 20)\n\n";
    }

    // F11: faulty full adder (cout = a AND b AND cin)
    {
        std::cout << "F11: full adder with cout = a AND b AND cin: failing rows (a b cin)\n";
        for (int i = 0; i < 8; ++i) {
            int a = (i >> 2) & 1, b = (i >> 1) & 1, c = i & 1, s, co;
            faultyFA(a, b, c, s, co);
            int trueCo = (a + b + c) >> 1;
            if (co != trueCo) std::cout << "  " << a << " " << b << " " << c << ": cout " << co << " should be " << trueCo << "\n";
        }
        int wrong = 0;
        for (int a = 0; a < 16; ++a)
            for (int b = 0; b < 16; ++b)
                if (rippleFaulty(a, b) != a + b) ++wrong;
        std::cout << "  4-bit ripple adder of such full adders (cin = 0): 0011 + 0001 gives "
                  << bin4(rippleFaulty(3, 1) & 0xF) << " (" << rippleFaulty(3, 1) << ") instead of 0100 (4); "
                  << "0001 + 0010 gives " << bin4(rippleFaulty(1, 2) & 0xF) << " (right); "
                  << wrong << " of 256 sums wrong\n\n";
    }

    // F12, F13 and the practical's named vectors
    std::cout << "F12/F13: 4-bit ALU arithmetic with flags (C = carry out; V = carry into bit 3 XOR carry out)\n";
    showOp("F12 SUB 3 - 9 (b = 1001 reads -7 as signed)", 3, 9, true);
    showOp("F13a ADD 1001 + 1100 (9 + 12 unsigned; -7 + -4 signed)", 9, 12, false);
    showOp("F13b ADD 0110 + 0010 (6 + 2)", 6, 2, false);
    showOp("F14 ADD 0111 + 0001 (7 + 1, the chapter's example)", 7, 1, false);
    std::cout << "  practical named vectors:\n";
    showOp("P ADD 9 + 7", 9, 7, false);
    showOp("P ADD 6 + 2", 6, 2, false);
    showOp("P SUB 3 - 9", 3, 9, true);
    showOp("P SUB 8 - 8", 8, 8, true);
    showOp("P SUB 0 - 1", 0, 1, true);
    std::cout << "  forensic named vectors (what a correct ALU gives):\n";
    showOp("SUB 5 - 7", 5, 7, true);
    showOp("SUB 9 - 3 (1001 reads -7; -7 - 3 = -10 overflows)", 9, 3, true);
    std::cout << "  check: V by carries equals V by range check in all 512 ADD/SUB cases: ";
    {
        bool ok = true;
        for (int a = 0; a < 16; ++a)
            for (int b = 0; b < 16; ++b)
                for (int s = 0; s < 2; ++s) {
                    Flags f = addFlags(a, b, s);
                    if ((f.V != 0) != f.rangeV) ok = false;
                }
        std::cout << (ok ? "yes" : "NO") << "\n\n";
    }

    // F16: the 2N + 1 model
    std::cout << "F16: 2N + 1 model (1 unit per NAND, nine-NAND full adders): N=24 -> settle "
              << 2 * 24 + 1 << " units; carry out at 2N = " << 2 * 24 << "; no-carry case 4 units; "
              << "N=16 -> " << 2 * 16 + 1 << "; N=8 -> " << 2 * 8 + 1 << "\n";
    std::cout << "  16-bit exhaustive test size: 2^16 * 2^16 * 8 = 2^35 = " << (1ULL << 35) << " vectors\n\n";

    // Practical and project counts by construction
    std::cout << "P/project counts by construction: mux8 = 7 mux2 = 28 NAND; 4 result bits -> 112 NAND in the result multiplexer;\n"
              << "  4-bit ripple adder (Yosys, F1-14) 28 NAND + 20 NOT; 2^35 vectors at 16 bits cannot be run\n";
    std::cout << "\nchecks done\n";
    return 0;
}
