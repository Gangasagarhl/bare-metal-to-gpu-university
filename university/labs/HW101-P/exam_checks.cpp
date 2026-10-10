// HW101 exam number check (Lab Engineer): recomputes every number used in the midterm, final and
// practical answer keys (university/_keys/HW101.keys.html). Exercise values throughout.
#include <cmath>
#include <iomanip>
#include <iostream>

static double par(double a, double b) { return a * b / (a + b); }
static void fit(const char* tag, const double v[], const double mA[], int n)
{
    double sVI = 0.0, sII = 0.0;
    std::cout << tag << " per pair:";
    for (int i = 0; i < n; ++i) {
        const double a = mA[i] / 1000.0;
        std::cout << " " << std::setprecision(1) << v[i] / a;
        sVI += v[i] * a;
        sII += a * a;
    }
    std::cout << " ohms; sum(VI) = " << std::setprecision(6) << sVI << ", sum(II) = " << std::scientific << sII
              << std::fixed << ", fit = " << std::setprecision(1) << sVI / sII << " ohms\n";
}

int main()
{
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "== Midterm (practice paper) ==\n";
    std::cout << "M2: 250 mA for 120 s: Q = " << 0.250 * 120.0 << " C\n";
    std::cout << "M3: red B black A: " << 7.5 - 12.0 << " V; red A black GND: " << 12.0 - 0.0 << " V; across A-B: " << 12.0 - 7.5 << " V\n";
    std::cout << "M4: 4.95 V / 3.3 kohm = " << 4.95 / 3300.0 * 1000.0 << " mA; 2.5 mA x 2.2 kohm = " << 0.0025 * 2200.0 << " V\n";
    { const double v[] = {2, 4, 6}, i[] = {0.60, 1.22, 1.79}; fit("M5:", v, i, 3); }
    std::cout << std::setprecision(3) << "M6: 220 ohm at 40 mA: V = " << 0.040 * 220.0 << " V, P = " << 0.040 * 0.040 * 220.0
              << " W = " << 0.040 * 0.040 * 220.0 / 0.25 * 100.0 << " % of 0.25 W; energy in 30 s = " << 0.040 * 0.040 * 220.0 * 30.0 << " J\n";
    std::cout << "M8: series 1.5k + 3k = " << 4500.0 << " ohms, I = " << 9.0 / 4500.0 * 1000.0 << " mA, V = " << 9.0 / 4500.0 * 1500.0 << " and "
              << 9.0 / 4500.0 * 3000.0 << " V; parallel I = " << 9.0 / 1500.0 * 1000.0 << " + " << 9.0 / 3000.0 * 1000.0 << " = "
              << 9.0 / 1500.0 * 1000.0 + 9.0 / 3000.0 * 1000.0 << " mA, Req = " << par(1500.0, 3000.0) << " ohms\n";
    std::cout << "M9: 10k/10k on 9 V: " << 9.0 * 0.5 << " V; with 20 kohm load: bottom " << par(10000.0, 20000.0) << " ohms, Vout = "
              << 9.0 * par(10000.0, 20000.0) / (10000.0 + par(10000.0, 20000.0)) << " V\n";
    std::cout << "M11: 35 - 12 = " << 35 - 12 << " mA; 9 V x 9 mA = " << 9.0 * 9.0 << " mW\n";

    std::cout << "== Final (practice paper) ==\n";
    std::cout << "F2: 120 mA for 45 s: Q = " << 0.120 * 45.0 << " C\n";
    std::cout << "F3: red GND black B: " << 0.0 - 5.5 << " V; red A black B: " << 9.0 - 5.5 << " V; drops " << 9.0 - 5.5 << " + " << 5.5 << " = " << 9.0 << " V\n";
    { const double v[] = {2, 4, 6}, i[] = {0.85, 1.72, 2.55}; fit("F4:", v, i, 3); }
    std::cout << std::setprecision(1) << "F5: part X: " << 1.0 / 0.002 << ", " << 3.0 / 0.006 << ", " << 5.0 / 0.010 << " ohms (ohmic); part Y: "
              << 1.0 / 0.0001 << " and " << 2.0 / 0.005 << " ohms (not ohmic)\n" << std::setprecision(3);
    std::cout << "F6: 9 V across 330 ohm: I = " << 9.0 / 330.0 * 1000.0 << " mA, P = " << std::setprecision(4) << 81.0 / 330.0 << " W = "
              << std::setprecision(1) << 81.0 / 330.0 / 0.25 * 100.0 << " % of 0.25 W; energy in 600 s = " << 81.0 / 330.0 * 600.0 << " J; with 680 ohm: P = "
              << std::setprecision(4) << 81.0 / 680.0 << " W = " << std::setprecision(1) << 81.0 / 680.0 / 0.25 * 100.0 << " %\n" << std::setprecision(3);
    std::cout << "F8: series 2.2k + 3.3k = " << 5500.0 << " ohms, I = " << 12.0 / 5500.0 * 1000.0 << " mA, V = " << 12.0 / 5500.0 * 2200.0 << " and "
              << 12.0 / 5500.0 * 3300.0 << " V; parallel I = " << 12.0 / 2200.0 * 1000.0 << " + " << 12.0 / 3300.0 * 1000.0 << " = "
              << 12.0 / 2200.0 * 1000.0 + 12.0 / 3300.0 * 1000.0 << " mA, Req = " << par(2200.0, 3300.0) << " ohms\n";
    {
        const double b1 = par(4700.0, 4700.0), b2 = par(4700.0, 47000.0);
        std::cout << "F9: 4.7k/4.7k on 5 V: " << 2.5 << " V; load 4.7k: bottom " << b1 << ", Vout = " << 5.0 * b1 / (4700.0 + b1)
                  << " V (" << std::setprecision(1) << (5.0 * b1 / (4700.0 + b1) - 2.5) / 2.5 * 100.0 << " %); load 47k: bottom " << std::setprecision(1) << b2
                  << ", Vout = " << std::setprecision(3) << 5.0 * b2 / (4700.0 + b2) << " V (" << std::setprecision(1)
                  << (5.0 * b2 / (4700.0 + b2) - 2.5) / 2.5 * 100.0 << " %); divider current " << std::setprecision(3) << 5.0 / 9400.0 * 1000.0 << " mA\n";
    }
    {
        const double tau = 22000.0 * 47e-6;
        std::cout << "F10: tau = " << tau << " s; Vc(tau) from 9 V = " << 9.0 * (1.0 - std::exp(-1.0)) << " V; t to 4.5 V = " << tau * std::log(2.0)
                  << " s; after 5 tau " << std::setprecision(1) << 100.0 * (1.0 - std::exp(-5.0)) << " %\n" << std::setprecision(3);
    }
    std::cout << "F12: 12 V, two LEDs 2 V, 15 mA: R = " << (12.0 - 4.0) / 0.015 << " ohms -> 560; I = " << 8.0 / 560.0 * 1000.0 << " mA; P resistor = "
              << std::setprecision(4) << 8.0 * 8.0 / 560.0 << " W; P each LED = " << 2.0 * 8.0 / 560.0 << " W\n" << std::setprecision(3);
    std::cout << "F13: 220 ohm: curve 14.273 vs simple 13.636 mA: simple is " << std::setprecision(1) << (14.273 - 13.636) / 14.273 * 100.0
              << " % low; 1 ohm: " << 2874.873 / 20.0 << " and " << 3000.0 / 20.0 << " times a 20 mA maximum\n" << std::setprecision(3);
    {
        const double b1 = par(220000.0, 1e6), b2 = par(220000.0, 1e7);
        std::cout << "F15: 220k/220k on 6 V: Rm 1 M: bottom " << std::setprecision(0) << b1 << ", reading " << std::setprecision(3) << 6.0 * b1 / (220000.0 + b1)
                  << " V (" << std::setprecision(1) << (6.0 * b1 / (220000.0 + b1) - 3.0) / 3.0 * 100.0 << " %); Rm 10 M: bottom " << std::setprecision(0) << b2
                  << ", reading " << std::setprecision(3) << 6.0 * b2 / (220000.0 + b2) << " V (" << std::setprecision(1)
                  << (6.0 * b2 / (220000.0 + b2) - 3.0) / 3.0 * 100.0 << " %)\n" << std::setprecision(3);
    }
    std::cout << "F16: 1.9 V / 0.625 = " << 1.9 / 0.625 << " -> code " << static_cast<int>(1.9 / 0.625) << "; 4.4 V / 0.625 = " << 4.4 / 0.625
              << " -> code " << static_cast<int>(4.4 / 0.625) << "; 1-bit at 2.5 V: " << (1.9 > 2.5) << " and " << (4.4 > 2.5) << "\n";
    {
        const double rc = 2.2, tH = rc * std::log((5.0 - 1.8) / (5.0 - 2.8)), tL = rc * std::log(2.8 / 1.8);
        std::cout << std::setprecision(4) << "F17: tH = " << tH << " s, tL = " << tL << " s, period " << tH + tL << " s, first HIGH "
                  << rc * std::log(5.0 / (5.0 - 2.8)) << " s\n" << std::setprecision(3);
    }
    std::cout << "F19: predicted 7 V / 1 kohm = " << 7.0 / 1000.0 * 1000.0 << " mA; CH2 R = " << 7.0 / 0.070 << " ohms, P = " << 7.0 * 0.070
              << " W; CH3 R = " << std::setprecision(0) << 7.0 / 0.0007 << " ohms, P = " << std::setprecision(4) << 7.0 * 0.0007
              << " W; correct channel P = " << 7.0 * 0.007 << " W; rating 0.25 W: CH2 " << std::setprecision(0) << 7.0 * 0.070 / 0.25 * 100.0 << " %\n"
              << std::setprecision(3);
    std::cout << "F20: 9 V, VF 2 V, 10 mA: R = " << 7.0 / 0.010 << " ohms -> next above from {470, 560, 680, 820}: 820; I = " << 7.0 / 820.0 * 1000.0
              << " mA; P resistor = " << std::setprecision(4) << 7.0 * 7.0 / 820.0 << " W\n" << std::setprecision(3);

    std::cout << "== Practical ==\n";
    std::cout << "P: minimum R = " << (6.0 - 2.0) / 0.010 << " -> 470; I = " << 4.0 / 470.0 * 1000.0 << " mA; with 1 ohm burden " << 4.0 / 471.0 * 1000.0
              << " mA (" << std::setprecision(2) << (4.0 / 471.0 - 4.0 / 470.0) / (4.0 / 470.0) * 100.0 << " %); tau = " << std::setprecision(3) << 47000.0 * 100e-6
              << " s; Vc(tau) = " << 6.0 * (1.0 - std::exp(-1.0)) << " V; t to 3 V = " << 4.7 * std::log(2.0) << " s; metered final "
              << 6.0 * 1e7 / (47000.0 + 1e7) << " V, metered tau " << par(47000.0, 1e7) * 100e-6 << " s, t to 3 V = "
              << par(47000.0, 1e7) * 100e-6 * std::log(6.0 * 1e7 / (47000.0 + 1e7) / (6.0 * 1e7 / (47000.0 + 1e7) - 3.0)) << " s\n";
    std::cout << "P case 2: minimum R = " << (9.0 - 2.0) / 0.015 << " -> 470; I = " << 7.0 / 470.0 * 1000.0 << " mA; tau = " << 22000.0 * 220e-6
              << " s; t to 4.5 V = " << 22000.0 * 220e-6 * std::log(2.0) << " s\n";
    return 0;
}
