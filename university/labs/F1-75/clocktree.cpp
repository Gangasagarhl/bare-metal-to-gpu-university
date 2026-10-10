// F1-75 Listing 1: a clock-tree calculator for the university's teaching chip "U-MCU-1".
// U-MCU-1 is NOT a real part: its oscillators, limits and rules are defined in this file
// (the constants and comments below) so that every number in the chapter can be checked.
// Reads configurations from standard input and prints every clock in the tree, the limit
// checks, and the UART baud rate the hardware really produces.
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

// ---- the teaching chip's own "datasheet" (our definitions, not a real product) ----
constexpr double kIrcHz = 8'000'000.0;        // internal RC oscillator
constexpr double kPllInMinHz = 1'000'000.0;   // PLL input range
constexpr double kPllInMaxHz = 25'000'000.0;
constexpr double kSysMaxHz = 72'000'000.0;    // maximum SYSCLK and AHB clock
constexpr double kApb1MaxHz = 36'000'000.0;   // maximum APB1 clock
constexpr double kApb2MaxHz = 72'000'000.0;   // maximum APB2 clock

struct Peripheral {
    std::string bus;      // "apb1" or "apb2"
    bool enabled = false; // the clock-enable (gate) bit
    double baud = 0.0;    // requested baud rate (UARTs only)
    double assumedHz = 0; // the bus clock the firmware ASSUMED when it computed the divisor
};

struct Config {
    std::string name;
    double xtalHz = 0.0;            // external crystal, 0 = not fitted
    std::string pllSrc = "irc/2";   // "irc/2" or "xtal"
    int pllMul = 2;
    std::string sysSrc = "irc";     // "irc", "xtal" or "pll"
    int ahbDiv = 1, apb1Div = 1, apb2Div = 1;
    std::map<std::string, Peripheral> periph{
        {"uart1", {"apb2"}}, {"uart2", {"apb1"}}, {"tim2", {"apb1"}}};
};

void line(const std::string& what, double hz, const std::string& note = "")
{
    std::cout << "  " << std::left << std::setw(22) << what << std::right << std::setw(12)
              << static_cast<long long>(std::llround(hz)) << " Hz" << (note.empty() ? "" : "  ")
              << note << '\n';
}

int evaluate(const Config& c)
{
    int problems = 0;
    std::cout << "config " << c.name << '\n';
    double pllIn = (c.pllSrc == "xtal") ? c.xtalHz : kIrcHz / 2.0;
    double pllOut = pllIn * c.pllMul;
    double sys = (c.sysSrc == "pll") ? pllOut : (c.sysSrc == "xtal") ? c.xtalHz : kIrcHz;
    if ((c.sysSrc == "xtal" || c.pllSrc == "xtal") && c.xtalHz == 0.0) {
        std::cout << "  PROBLEM: the crystal is selected but none is fitted (xtal 0)\n";
        ++problems;
    }
    line("IRC (internal RC)", kIrcHz);
    line("XTAL (crystal)", c.xtalHz, c.xtalHz == 0.0 ? "(not fitted)" : "");
    if (c.sysSrc == "pll") {
        line("PLL input (" + c.pllSrc + ")", pllIn);
        line("PLL output (x" + std::to_string(c.pllMul) + ")", pllOut);
        if (pllIn < kPllInMinHz || pllIn > kPllInMaxHz) {
            std::cout << "  PROBLEM: PLL input outside 1-25 MHz\n";
            ++problems;
        }
    }
    const double ahb = sys / c.ahbDiv;
    const double apb1 = ahb / c.apb1Div;
    const double apb2 = ahb / c.apb2Div;
    // Rule of this chip: a timer on an APB bus runs at the bus clock when the APB divider
    // is 1, and at twice the bus clock otherwise.
    const double tim2 = (c.apb1Div == 1) ? apb1 : 2.0 * apb1;
    line("SYSCLK (" + c.sysSrc + ")", sys, sys > kSysMaxHz ? "PROBLEM: above 72 MHz" : "ok");
    line("AHB (/" + std::to_string(c.ahbDiv) + ")", ahb);
    line("APB1 (/" + std::to_string(c.apb1Div) + ")", apb1, apb1 > kApb1MaxHz ? "PROBLEM: above 36 MHz" : "ok");
    line("APB2 (/" + std::to_string(c.apb2Div) + ")", apb2, apb2 > kApb2MaxHz ? "PROBLEM: above 72 MHz" : "ok");
    problems += (sys > kSysMaxHz) + (apb1 > kApb1MaxHz) + (apb2 > kApb2MaxHz);

    for (const auto& [name, p] : c.periph) {
        const double busHz = (p.bus == "apb1") ? apb1 : apb2;
        const double clk = (name == "tim2") ? tim2 : busHz;
        if (!p.enabled) {
            std::cout << "  " << std::left << std::setw(22) << (name + " clock") << std::right
                      << std::setw(12) << 0 << " Hz  gate OFF: in this model its registers read 0"
                      << " and writes are ignored\n";
            continue;
        }
        line(name + " clock (" + p.bus + ")", clk, "gate on");
        if (p.baud > 0.0) {
            const double basis = (p.assumedHz > 0.0) ? p.assumedHz : clk;
            const long div = std::lround(basis / p.baud);   // what the firmware writes
            const double actual = clk / static_cast<double>(div);
            const double err = 100.0 * (actual - p.baud) / p.baud;
            std::cout << "  " << name << ": divisor " << div << " (computed for "
                      << static_cast<long long>(std::llround(basis)) << " Hz), real baud "
                      << static_cast<long long>(std::llround(actual)) << ", error "
                      << std::showpos << std::fixed << std::setprecision(2) << err
                      << std::noshowpos << std::defaultfloat << " %";
            if (std::fabs(err) > 2.0) {
                std::cout << "  PROBLEM: more than 2 % off";
                ++problems;
            }
            std::cout << '\n';
        }
    }
    std::cout << "  problems: " << problems << "\n\n";
    return problems;
}

}  // namespace

int main()
{
    std::vector<Config> configs;
    std::string word;
    std::string text;
    while (std::getline(std::cin, text)) {
        if (text.empty() || text[0] == '#') {
            continue;
        }
        std::istringstream in(text);
        in >> word;
        if (word == "config") {
            configs.emplace_back();
            in >> configs.back().name;
            continue;
        }
        if (configs.empty()) {
            std::cerr << "error: '" << word << "' before any 'config' line\n";
            return 2;
        }
        Config& c = configs.back();
        std::string name;
        if (word == "xtal") { in >> c.xtalHz; }
        else if (word == "pllsrc") { in >> c.pllSrc; }
        else if (word == "pllmul") { in >> c.pllMul; }
        else if (word == "sysclk") { in >> c.sysSrc; }
        else if (word == "ahb") { in >> c.ahbDiv; }
        else if (word == "apb1") { in >> c.apb1Div; }
        else if (word == "apb2") { in >> c.apb2Div; }
        else if (word == "enable") { in >> name; c.periph.at(name).enabled = true; }
        else if (word == "baud") { in >> name; in >> c.periph.at(name).baud; }
        else if (word == "assume") { in >> name; in >> c.periph.at(name).assumedHz; }
        else {
            std::cerr << "error: unknown keyword '" << word << "'\n";
            return 2;
        }
    }
    int total = 0;
    for (const Config& c : configs) {
        total += evaluate(c);
    }
    std::cout << "configurations: " << configs.size() << ", problems in total: " << total << '\n';
    return 0;
}
