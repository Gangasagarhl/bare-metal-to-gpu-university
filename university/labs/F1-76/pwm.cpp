// F1-76 Listing 1: a model of the PWM timer of the university's teaching chip U-MCU-1
// (not a real part; its rules are defined here and in the F1-78 reference-manual excerpt).
//   counter clock  f_cnt = f_tim / (PSC + 1)
//   the counter goes 0, 1, ..., ARR, then wraps to 0 (an "update event")
//   PWM output is high while CNT < CCR, low otherwise
//   so: period = (ARR + 1) / f_cnt and duty = CCR / (ARR + 1), clamped to 0..100 %
// Input lines:
//   show  <name> <f_tim Hz> <PSC> <ARR> <CCR>   compute frequency and duty, draw small cases
//   solve <name> <f_tim Hz> <target Hz> <duty %>  find PSC, ARR, CCR (16-bit registers)
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

constexpr std::uint32_t kMax16 = 65535;

void show(const std::string& name, double fTim, std::uint32_t psc, std::uint32_t arr,
          std::uint32_t ccr)
{
    const double fCnt = fTim / (psc + 1.0);
    const double freq = fCnt / (arr + 1.0);
    const double duty = (ccr > arr) ? 100.0 : 100.0 * ccr / (arr + 1.0);
    std::cout << name << ": PSC=" << psc << " ARR=" << arr << " CCR=" << ccr
              << "  counter " << std::fixed << std::setprecision(1) << fCnt << " Hz"
              << ", PWM " << std::setprecision(3) << freq << " Hz"
              << ", period " << std::setprecision(3) << 1e6 / freq << " us"
              << ", high " << std::setprecision(3) << 1e6 * (std::min(ccr, arr + 1) / fCnt) << " us"
              << ", duty " << std::setprecision(2) << duty << " %\n";
    if (arr < 20) {   // draw two periods, one character per counter tick
        std::cout << "  CNT : ";
        for (std::uint32_t t = 0; t < 2 * (arr + 1); ++t) {
            std::cout << (t % (arr + 1)) % 10;
        }
        std::cout << "\n  OUT : ";
        for (std::uint32_t t = 0; t < 2 * (arr + 1); ++t) {
            std::cout << (((t % (arr + 1)) < ccr) ? '#' : '_');
        }
        std::cout << "\n";
    }
}

void solve(const std::string& name, double fTim, double target, double dutyPct)
{
    // The smallest prescaler that lets ARR fit in 16 bits gives the finest duty steps.
    for (std::uint32_t psc = 0; psc <= kMax16; ++psc) {
        const double ticks = fTim / (psc + 1.0) / target;   // = ARR + 1
        if (ticks <= kMax16 + 1.0) {
            const auto arr = static_cast<std::uint32_t>(ticks + 0.5) - 1;
            const auto ccr = static_cast<std::uint32_t>(dutyPct / 100.0 * (arr + 1) + 0.5);
            std::cout << name << ": target " << target << " Hz, " << dutyPct << " % -> ";
            show("solution", fTim, psc, arr, ccr);
            return;
        }
    }
    std::cout << name << ": no 16-bit solution\n";
}

}  // namespace

int main()
{
    std::string text;
    while (std::getline(std::cin, text)) {
        if (text.empty() || text[0] == '#') {
            continue;
        }
        std::istringstream in(text);
        std::string cmd, name;
        in >> cmd >> name;
        if (cmd == "show") {
            double f = 0;
            std::uint32_t psc = 0, arr = 0, ccr = 0;
            in >> f >> psc >> arr >> ccr;
            show(name, f, psc, arr, ccr);
        } else if (cmd == "solve") {
            double f = 0, target = 0, duty = 0;
            in >> f >> target >> duty;
            solve(name, f, target, duty);
        } else {
            std::cerr << "unknown command: " << cmd << '\n';
            return 2;
        }
    }
    return 0;
}
