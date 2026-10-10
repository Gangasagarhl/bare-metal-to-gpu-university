// MA102 exam number check (Lab Engineer): recomputes every number used in the final, practical and
// project answer keys (university/_keys/MA102.keys.html). Every value is computed, none is typed in.
#include <bitset>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

static std::string bin(unsigned v, int width) { return std::bitset<16>(v).to_string().substr(static_cast<std::size_t>(16 - width)); }
static std::string hex2(unsigned v) { const char* d = "0123456789ABCDEF"; std::string s; s += d[(v >> 4) & 0xF]; s += d[v & 0xF]; return s; }
static unsigned fromBin(const std::string& s) { unsigned v = 0; for (char c : s) v = v * 2 + static_cast<unsigned>(c - '0'); return v; }
static int asSigned(unsigned byte) { return static_cast<std::int8_t>(static_cast<std::uint8_t>(byte)); }
static unsigned pow2(int n) { return 1u << n; }

int main()
{
    std::cout << "== Final ==\n";
    std::cout << "F1: place values of six switches from the right: ";
    for (int k = 0; k < 6; ++k) std::cout << pow2(k) << (k < 5 ? ", " : "\n");
    std::cout << "F2: 10110 = " << fromBin("10110") << "; 110011 = " << fromBin("110011") << "; 19 = " << bin(19, 5) << "\n";
    std::cout << "F3: 7 switches: " << pow2(7) << " patterns, biggest " << pow2(7) - 1 << "\n";
    { unsigned n = 77; std::cout << "F4: 77 ladder:"; while (n > 0) { std::cout << " " << n << "/2=" << n / 2 << " r" << n % 2 << ";"; n = n / 2; }
      std::cout << " 77 = " << bin(77, 7) << "; doubling 1011010:"; unsigned v = 0; for (char c : std::string("1011010")) { v = v * 2 + static_cast<unsigned>(c - '0'); std::cout << " " << v; }
      std::cout << " -> " << fromBin("1011010") << "\n"; }
    { std::string r12 = bin(12, 4); std::string rev(r12.rbegin(), r12.rend()); std::string r9 = bin(9, 4); std::string rev9(r9.rbegin(), r9.rend());
      std::cout << "F5: 12 = " << r12 << ", reversed " << rev << " = " << fromBin(rev) << "; 9 = " << r9 << ", reversed " << rev9 << " (same)\n"; }
    std::cout << "F6: 1110 = " << fromBin("1110") << " = hex " << hex2(fromBin("1110")).substr(1) << "\n";
    std::cout << "F7: 10111001 = 0x" << hex2(fromBin("10111001")) << " = " << fromBin("10111001") << "; 0x7C = " << bin(0x7C, 8) << " = " << 0x7C
              << "; 203 / 16 = " << 203 / 16 << " r " << 203 % 16 << " -> 0x" << hex2(203) << "\n";
    std::cout << "F8: red 255 green 8 blue 0 -> #" << hex2(255) << hex2(8) << hex2(0) << " (8 printed with one digit: " << std::hex << std::uppercase << 8 << std::dec << ")\n";
    { const long long kB = 1000, KiB = 1024; std::cout << "F9: 1 kB = " << kB << " B, 1 KiB = " << KiB << " B, 3 MiB = " << 3 * KiB * KiB << " B\n";
      const long long f = 2097152; std::cout << "F10: " << f << " B = " << static_cast<double>(f) / (KiB * KiB) << " MiB = " << std::setprecision(7) << static_cast<double>(f) / (kB * kB) << std::setprecision(6) << " MB = " << f * 8 << " bits\n";
      std::cout << "F11: 250 kB = " << 250 * kB << " B = " << std::setprecision(9) << 250.0 * static_cast<double>(kB) / static_cast<double>(KiB) << std::setprecision(6) << " KiB\n"; }
    std::cout << "F12: 11111111 unsigned " << fromBin("11111111") << ", signed " << asSigned(fromBin("11111111")) << "\n";
    { unsigned twenty = 20; unsigned flipped = (~twenty) & 0xFFu; unsigned plusOne = (flipped + 1) & 0xFFu;
      std::cout << "F13: 20 = " << bin(twenty, 8) << ", flipped " << bin(flipped, 8) << ", plus one " << bin(plusOne, 8) << " = 0x" << hex2(plusOne) << " reads " << asSigned(plusOne)
                << "; 10010110 unsigned " << fromBin("10010110") << " signed " << asSigned(fromBin("10010110")) << "; 100 + 100 = " << 100 + 100 << " in a signed byte reads " << asSigned(200) << "\n"; }
    std::cout << "F14: 10010100 signed = " << asSigned(fromBin("10010100")) << " (unsigned " << fromBin("10010100") << ")\n";
    { unsigned x = 0b0110'1001u;
      std::cout << "F15: x = " << bin(x, 8) << " = 0x" << hex2(x) << " = " << x << "; set bit 4: " << bin(x | (1u << 4), 8) << "; clear bit 0: " << bin(x & ~(1u << 0), 8)
                << "; toggle bit 7: " << bin(x ^ (1u << 7), 8) << "; x >> 2 = " << bin(x >> 2, 8) << ", (x >> 2) & 0b11 = " << ((x >> 2) & 0b11u) << "; 1u << 6 = " << (1u << 6) << "\n"; }
    { unsigned x = 0b0000'0100u; unsigned y = 0b0000'0001u;
      std::cout << "F16: 4 != 0 is " << (4 != 0) << ", so x & (4 != 0) tests bit 0: for x = " << bin(x, 8) << " it gives " << (x & (4 != 0)) << ", for " << bin(y, 8) << " it gives " << (y & (4 != 0))
                << "; (x & 4) != 0 for " << bin(x, 8) << " gives " << ((x & 4u) != 0) << "\n"; }
    std::cout << "F18: XOR rows 00 01 10 11 -> " << (0 ^ 0) << " " << (0 ^ 1) << " " << (1 ^ 0) << " " << (1 ^ 1) << "\n";
    std::cout << "F19: row 1,1: OR " << (1 | 1) << ", XOR " << (1 ^ 1) << "\n";
    { int lit = 0; std::cout << "F20: A AND NOT (B OR C), rows ABC:";
      for (int a = 0; a < 2; ++a) for (int b = 0; b < 2; ++b) for (int c = 0; c < 2; ++c) { const bool lamp = a && !(b || c); lit += lamp ? 1 : 0; std::cout << " " << a << b << c << "->" << lamp; }
      std::cout << "; rows lit: " << lit << " of 8\n"; }
    { int fail1 = 0, fail2 = 0; std::cout << "F21: rows AB:";
      for (int a = 0; a < 2; ++a) for (int b = 0; b < 2; ++b) { const bool l1 = !(a && !b); const bool r1 = !a || b; const bool l2 = (a && b) || (a && !b); const bool r2 = a;
          std::cout << " " << a << b << ":" << l1 << r1 << "," << l2 << r2; fail1 += (l1 != r1); fail2 += (l2 != r2); }
      std::cout << "; NOT(A AND NOT B) vs NOT A OR B failures " << fail1 << "; (A AND B) OR (A AND NOT B) vs A failures " << fail2 << "\n"; }
    { int fails = 0; std::cout << "F22: NOT(A OR B) vs NOT A OR NOT B, rows AB:";
      for (int a = 0; a < 2; ++a) for (int b = 0; b < 2; ++b) { const bool l = !(a || b); const bool r = !a || !b; std::cout << " " << a << b << ":" << l << r; fails += (l != r); }
      std::cout << "; fails in " << fails << " rows\n"; }
    { int counter = 14; std::cout << "F25: start " << counter << " (even: " << (counter % 2 == 0) << "); +4 keeps parity: " << ((counter + 4) % 2 == 0) << "; -10 keeps parity: " << ((counter - 10) % 2 == 0)
                << "; 21 is even: " << (21 % 2 == 0) << "\n"; }
    { const int original = 45; int rest = original / 2; int placeValue = 1 * 3; int doneSoFar = 0 + (original % 2) * 1;
      std::cout << "F26: second printed line with placeValue * 3: rest=" << rest << " placeValue=" << placeValue << " doneSoFar=" << doneSoFar << ", rest * placeValue + doneSoFar = " << rest * placeValue + doneSoFar
                << ", holds: " << (original == rest * placeValue + doneSoFar) << "\n"; }
    std::cout << "F27 (forensic): bytes 04 00 FD F6 EC E2 unsigned:";
    for (unsigned b : {0x04u, 0x00u, 0xFDu, 0xF6u, 0xECu, 0xE2u}) std::cout << " " << b;
    std::cout << "; signed:";
    for (unsigned b : {0x04u, 0x00u, 0xFDu, 0xF6u, 0xECu, 0xE2u}) std::cout << " " << asSigned(b);
    std::cout << "; wrong ones minus 256:";
    for (unsigned b : {0xFDu, 0xF6u, 0xECu, 0xE2u}) std::cout << " " << static_cast<int>(b) - 256;
    std::cout << "\n";

    std::cout << "== Practical ==\n";
    for (unsigned s : {0x5Au, 0xE3u, 0x0Du, 0xABu}) {
        std::cout << "P: GSTAT 0x" << hex2(s) << " = " << bin(s, 8) << ": ALARM " << ((s >> 7) & 1u) << ", ZONE " << ((s >> 4) & 0b111u) << ", PUMP " << ((s & (1u << 3)) != 0)
                  << ", LID " << ((s & (1u << 2)) != 0) << ", LEVEL " << (s & 0b11u) << "; wide mask (status >> 4) & 0b1111 gives " << ((s >> 4) & 0b1111u) << "\n";
    }
    for (unsigned t : {0xF6u, 0x12u, 0xECu, 0x00u}) std::cout << "P: TEMP 0x" << hex2(t) << " = " << bin(t, 8) << " unsigned " << t << ", signed " << asSigned(t) << "\n";
    { unsigned enc = (1u << 7) | (2u << 4) | (1u << 3) | (0u << 2) | 3u; std::cout << "P: encode ALARM 1, ZONE 2, PUMP 1, LID 0, LEVEL 3 -> " << bin(enc, 8) << " = 0x" << hex2(enc) << " = " << enc << "\n"; }
    { std::cout << "P: masks: set PUMP on 0xE3 -> 0x" << hex2(0xE3u | (1u << 3)) << "; clear ALARM on 0xE3 -> 0x" << hex2(0xE3u & ~(1u << 7) & 0xFFu) << "; LID test on 0x0D -> " << ((0x0Du & (1u << 2)) != 0) << "\n"; }
    { const long long packets = 86400 / 2; const long long bytes = packets * 2;
      std::cout << "P: packets per day at one every 2 s: " << packets << "; bytes " << bytes << " = " << static_cast<double>(bytes) / 1024 << " KiB = " << static_cast<double>(bytes) / 1000 << " kB\n"; }

    std::cout << "== Project reference ==\n";
    std::cout << "patterns of one byte: " << pow2(8) << "; 0xFF = " << 0xFF << "; 2^16 round trips: " << (1u << 16) << "\n";
    return 0;
}
