// MA101 exam number check (Lab Engineer): recomputes every number used in the midterm, final,
// practical and project answer keys (university/_keys/MA101.keys.html), as the chapters' checks.cpp do.
#include <climits>
#include <iomanip>
#include <iostream>

static int roundUp(int jobs, int teamSize) { return (jobs + teamSize - 1) / teamSize; }
static int roundToTen(int n) { return (n + 5) / 10 * 10; }
static long long power(long long base, int n) { long long v = 1; for (int i = 0; i < n; ++i) v = v * base; return v; }
static int log2int(long long x) { int n = 0; while (x > 1) { x = x / 2; ++n; } return n; }

int main()
{
    std::cout << "== Midterm ==\n";
    std::cout << "M1: in 8245 the digit 2 is worth " << (8245 / 100 % 10) * 100 << "\n";
    std::cout << "M2: 6030 = " << 6000 << " + " << 0 << " + " << 30 << " + " << 0 << " = " << 6000 + 0 + 30 + 0 << "\n";
    std::cout << "M3: 253 beads, bags of ten: " << 253 / 10 << " full, " << 253 % 10 << " left, bags needed " << roundUp(253, 10) << "\n";
    std::cout << "M4: order: 998 < 2090: " << (998 < 2090) << ", 2090 < 2099: " << (2090 < 2099) << ", 2099 < 2900: " << (2099 < 2900) << "\n";
    std::cout << "M5: 4 + 3 * 5 = " << 4 + 3 * 5 << "; (4 + 3) * 5 = " << (4 + 3) * 5 << "; 4 + 3 + 5 = " << 4 + 3 + 5 << "; 4 * 3 * 5 = " << 4 * 3 * 5 << "\n";
    std::cout << "M6: 38 / 6 = " << 38 / 6 << " remainder " << 38 % 6 << "; check " << (38 / 6) * 6 + 38 % 6 << "; boxes " << roundUp(38, 6) << "\n";
    std::cout << "M7: 61 -> " << roundToTen(61) << ", 29 -> " << roundToTen(29) << ", estimate " << roundToTen(61) * roundToTen(29) << ", exact " << 61 * 29 << "\n";
    std::cout << "M8: 4 * 6 + 3 = " << 4 * 6 + 3 << "; 4 * (6 + 3) = " << 4 * (6 + 3) << "\n";
    std::cout << "M10: 3/5 = " << 3.0 / 5.0 << " = " << 3.0 / 5.0 * 100.0 << " %; 40 % of 70 = " << 70.0 / 100.0 * 40.0 << "\n";
    std::cout << "M11: 1/3 + 1/6 = 2/6 + 1/6 = 3/6 = " << 3.0 / 6.0 << "; 1/2 = " << 1.0 / 2.0 << "; 2/9 = " << 2.0 / 9.0 << "; 1/9 = " << 1.0 / 9.0 << "\n";
    { int x = 7 / 8; std::cout << "M12: int x = 7 / 8 prints " << x << "; 7.0 / 8.0 = " << 7.0 / 8.0 << "\n"; }
    std::cout << "M13: -6 + 4 = " << -6 + 4 << "; 3 - 8 = " << 3 - 8 << "; 2 - (-5) = " << 2 - (-5) << "\n";
    std::cout << "M14: -7 < -3: " << (-7 < -3) << "\n";
    std::cout << "M15: from floor -3 to floor 2: " << 2 - (-3) << " floors\n";
    { unsigned int a = 3; unsigned int b = 6; int c = 3; int d = 6;
      std::cout << "M16: 3 - 6 with unsigned int prints " << a - b << "; with int prints " << c - d << "\n"; }
    std::cout << "M17: 2^4 = " << power(2, 4) << "; 2 * 4 = " << 2 * 4 << "; 2 + 4 = " << 2 + 4 << "; 4 * 3 * 2 = " << 4 * 3 * 2 << "\n";
    std::cout << "M18: 7 switches: " << power(2, 7) << " patterns; log2(64) = " << log2int(64) << "; 10^5 = " << power(10, 5) << "\n";
    std::cout << "M19: 2^1 = " << power(2, 1) << ", halved once: " << power(2, 1) / 2 << " = 2^0\n";
    std::cout << "M20: 2000 jobs, teams of 256: teams " << roundUp(2000, 256) << ", helpers " << roundUp(2000, 256) * 256
              << ", idle " << roundUp(2000, 256) * 256 - 2000 << " (7 teams hold " << 7 * 256 << ")\n";
    std::cout << "M21: 5 * 8 - 4 = " << 5 * 8 - 4 << "\n";

    std::cout << "== Final ==\n";
    std::cout << "F1: in 70406 the digit 4 is worth " << (70406 / 100 % 10) * 100 << "\n";
    std::cout << "F2: 437 beans, cups of ten: " << 437 / 10 << " full, " << 437 % 10 << " left, cups needed " << roundUp(437, 10) << "\n";
    std::cout << "F3: 2 + 5 * (8 - 3) / 5 = " << 2 + 5 * (8 - 3) / 5 << "; 100 - 24 / 4 * 2 = " << 100 - 24 / 4 * 2 << "\n";
    std::cout << "F4: 48 -> " << roundToTen(48) << ", 52 -> " << roundToTen(52) << ", estimate " << roundToTen(48) * roundToTen(52)
              << ", exact " << 48 * 52 << ", the friend's 24960 / exact = " << 24960 / (48 * 52) << "\n";
    std::cout << "F5: 7/8 = " << 7.0 / 8.0 << " = " << 7.0 / 8.0 * 100.0 << " %; 15 % of 60 = " << 60.0 / 100.0 * 15.0
              << "; 3/4 + 1/8 = 6/8 + 1/8 = 7/8 = " << 7.0 / 8.0 << "\n";
    { double s = 0.1 + 0.2; std::cout << "F6: 0.1 + 0.2 prints " << s << " and with 17 digits " << std::setprecision(17) << s << std::setprecision(6) << "\n"; }
    std::cout << "F7: -9 + 4 = " << -9 + 4 << "; -3 - (-8) = " << -3 - (-8) << "; distance -12 to 7 = " << 7 - (-12) << "; (-3) * (-4) = " << (-3) * (-4) << "\n";
    { unsigned int z = 0; unsigned int one = 1; std::cout << "F8: 0 - 1 with unsigned int prints " << z - one << "; with int prints " << 0 - 1 << "\n"; }
    std::cout << "F9: 2^6 * 2^4 = " << power(2, 6) * power(2, 4) << " = 2^" << log2int(power(2, 6) * power(2, 4)) << "; log2(512) = " << log2int(512)
              << "; 10^5 = " << power(10, 5) << "; 2^10 - 10^3 = " << power(2, 10) - power(10, 3) << "\n";
    std::cout << "F10: 9 switches: " << power(2, 9) << " patterns; 8 switches: " << power(2, 8) << "\n";
    { int n = 6; int m = -2; int a = 1; int x = 2;
      std::cout << "F11: 4n - 3 at n = 6: " << 4 * n - 3 << "; at n = -2: " << 4 * m - 3 << "; 7a + 2 - 3a at a = 1: " << 7 * a + 2 - 3 * a
                << " and 4a + 2: " << 4 * a + 2 << "; 3(x + 5) at x = 2: " << 3 * (x + 5) << " and 3x + 15: " << 3 * x + 15 << "\n"; }
    std::cout << "F12: 6 seats per table: table 4 seat 5 -> ticket " << 4 * 6 + 5 << "; ticket 20 -> table " << 20 / 6 << ", seat " << 20 % 6 << "\n";
    std::cout << "F13: 3n + 2 at n = 7: " << 3 * 7 + 2 << " (digits read as 37 give " << 37 + 2 << ")\n";
    std::cout << "F14: 4x - 5 = 23: x = " << (23 + 5) / 4 << ", check " << 4 * ((23 + 5) / 4) - 5 << "; 2(x + 3) = 20: x = " << 20 / 2 - 3
              << ", check " << 2 * (20 / 2 - 3 + 3) << "; x / 5 = 6: x = " << 6 * 5 << "\n";
    std::cout << "F15: 12t = 3000: t = " << 3000 / 12 << ", check " << 12 * (3000 / 12) << "; 23 = table * 5 + 3: table = " << (23 - 3) / 5 << "\n";
    std::cout << "F16: 5x + 1 = 13: 5x = " << 13 - 1 << ", x = " << (13 - 1) / 5.0 << " (whole-number division gives " << (13 - 1) / 5 << " remainder " << (13 - 1) % 5 << ")\n";
    std::cout << "F17: d = 20 + 12t: t = 0..3 ->";
    for (int t = 0; t <= 3; ++t) std::cout << " " << 20 + 12 * t;
    std::cout << "; t = 5 -> " << 20 + 12 * 5 << "; d = 104 -> t = " << (104 - 20) / 12 << "\n";
    std::cout << "F18: 3 * 3 + 1 = " << 3 * 3 + 1 << "\n";
    std::cout << "F19: y = 30 - 5x at x = 4: " << 30 - 5 * 4 << "; y = 0 at x = " << 30 / 5 << "\n";
    { int c1 = 0; for (int i = 0; i < 25; ++i) ++c1; int c2 = 0; for (int i = 0; i <= 25; ++i) ++c2;
      std::cout << "F20: 0 <= i < 25 holds " << c1 << " numbers; 0 <= i <= 25 holds " << c2 << "; 3x + 2 < 14 -> 3x < " << 14 - 2
                << " -> x < " << (14 - 2) / 3 << "; -3x > 12 -> x < " << 12 / -3 << "; test x = -5: " << -3 * -5 << " > 12 is " << (-3 * -5 > 12) << "\n"; }
    { int jobs = 75; int size = 8; int teams = roundUp(jobs, size);
      std::cout << "F21: 75 chairs, teams of 8: teams " << teams << " (9 teams hold " << 9 * size << "), helpers " << teams * size
                << ", idle " << teams * size - jobs << ", last working ticket " << jobs - 1 << " = team " << (jobs - 1) / size << " seat " << (jobs - 1) % size
                << " (check " << ((jobs - 1) / size) * size + (jobs - 1) % size << "), first idle ticket " << jobs << "\n"; }
    std::cout << "F23 (forensic): 50 / 8 = " << 50 / 8 << " (rounded down), 6 * 8 = " << 6 * 8 << ", undone " << 50 - 48
              << "; rounded up " << roundUp(50, 8) << " teams, " << roundUp(50, 8) * 8 << " helpers, idle " << roundUp(50, 8) * 8 - 50 << "\n";
    std::cout << "F24: cost = 3k + 2: k = 0..3 ->";
    for (int k = 0; k <= 3; ++k) std::cout << " " << 3 * k + 2;
    std::cout << "\n";

    std::cout << "== Practical ==\n";
    for (int jobs : {300, 320, 321}) {
        int size = 32; int teams = roundUp(jobs, size);
        std::cout << "P: " << jobs << " jobs, teams of " << size << ": " << (jobs + size - 1) << " / " << size << " = " << teams
                  << " teams (" << (teams - 1) << " teams hold " << (teams - 1) * size << "), helpers " << teams * size << ", idle " << teams * size - jobs
                  << ", last working ticket " << jobs - 1 << " = team " << (jobs - 1) / size << " seat " << (jobs - 1) % size
                  << ", first idle ticket " << (teams * size > jobs ? jobs : -1) << "\n";
    }

    std::cout << "== Project reference ==\n";
    std::cout << "bits per byte on this machine (CHAR_BIT): " << CHAR_BIT << ", patterns per byte: " << power(2, CHAR_BIT) << "\n";
    for (int n = 0; n <= 20; ++n) std::cout << "2^" << n << " = " << power(2, n) << "\n";
    std::cout << "2^10 - 10^3 = " << power(2, 10) - power(10, 3) << "; 2^20 - 10^6 = " << power(2, 20) - power(10, 6) << "\n";
    std::cout << "log2 of 256, 1024, 65536: " << log2int(256) << " " << log2int(1024) << " " << log2int(65536) << "\n";
    return 0;
}
