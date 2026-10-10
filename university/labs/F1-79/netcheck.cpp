// F1-79 Listing 1: a schematic checker for the university's teaching board U-BOARD-1.
// It reads a netlist (the text form of a schematic) plus a few "datasheet" limits, and
// prints what a board-support note needs: what each LED and button is connected to, its
// active level, the LED currents, and the rule violations it finds.
// Simplifications (stated in F1-79): an output pin is ideal (HIGH = supply voltage,
// LOW = 0 V, no internal resistance); an LED drops exactly its forward voltage vf.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Two {             // a two-terminal part
    std::string kind, ref, a, b;   // for an LED: a = anode net, b = cathode net
    double value = 0.0;            // ohms (resistor), vf (LED)
    double imax = 0.0;             // LED maximum current
};

struct Board {
    std::string name;
    std::map<std::string, double> supply;          // net -> volts
    std::map<std::string, std::string> mcuPin;      // pin name -> net
    std::string mcuRef;
    std::vector<Two> parts;
    std::set<std::string> dnp;                      // refs marked "do not populate"
    std::map<std::string, std::string> fwPull;      // pin -> none|up|down (firmware setting)
    double pinMaxA = 0.0;                           // datasheet: max current per I/O pin
};

bool fitted(const Board& b, const Two& p) { return b.dnp.count(p.ref) == 0; }

std::string pinOnNet(const Board& b, const std::string& net)
{
    for (const auto& [pin, n] : b.mcuPin) {
        if (n == net && pin.rfind("P", 0) == 0) { return pin; }   // GPIO pins start with P
    }
    return "";
}

std::string otherEnd(const Two& r, const std::string& net)
{
    return (r.a == net) ? r.b : (r.b == net) ? r.a : "";
}

int checkLeds(const Board& b, double vdd)
{
    int problems = 0;
    for (const Two& d : b.parts) {
        if (d.kind != "led" || !fitted(b, d)) { continue; }
        // Look for: GPIO -> resistor -> anode ... cathode -> GND  (active-high), or
        //           VDD -> resistor -> anode ... cathode -> GPIO  (active-low)
        for (const Two& r : b.parts) {
            if (r.kind != "resistor" || !fitted(b, r)) { continue; }
            if (r.a != d.a && r.b != d.a && r.a != d.b && r.b != d.b) { continue; }
            const bool rOnAnode = (r.a == d.a || r.b == d.a);
            const std::string far = otherEnd(r, rOnAnode ? d.a : d.b);
            const std::string pinA = pinOnNet(b, far);
            const std::string pinK = pinOnNet(b, d.b);
            std::string pin, level;
            if (rOnAnode && !pinA.empty() && b.supply.count(d.b) && b.supply.at(d.b) == 0.0) {
                pin = pinA; level = "HIGH (active-high, the pin sources current)";
            } else if (rOnAnode && b.supply.count(far) && b.supply.at(far) > 0.0 && !pinK.empty()) {
                pin = pinK; level = "LOW (active-low, the pin sinks current)";
            } else {
                continue;
            }
            const double amps = (vdd - d.value) / r.value;
            std::cout << "  " << d.ref << " via " << r.ref << " (" << r.value << " ohm) on "
                      << pin << ": lights when " << pin << " is " << level << "\n"
                      << "      current = (" << vdd << " V - " << d.value << " V) / " << r.value
                      << " ohm = " << std::fixed << std::setprecision(1) << amps * 1000.0
                      << " mA" << std::defaultfloat << std::setprecision(6);
            if (amps > b.pinMaxA) {
                std::cout << "  PROBLEM: above the pin limit of " << b.pinMaxA * 1000.0 << " mA";
                ++problems;
            }
            if (amps > d.imax) {
                std::cout << "  PROBLEM: above the LED limit of " << d.imax * 1000.0 << " mA";
                ++problems;
            }
            std::cout << '\n';
        }
    }
    return problems;
}

int checkButtons(const Board& b)
{
    int problems = 0;
    for (const Two& s : b.parts) {
        if (s.kind != "button" || !fitted(b, s)) { continue; }
        const std::string sigNet = pinOnNet(b, s.a).empty() ? s.b : s.a;
        const std::string pin = pinOnNet(b, sigNet);
        const std::string otherNet = otherEnd(s, sigNet);
        const bool toGround = b.supply.count(otherNet) && b.supply.at(otherNet) == 0.0;
        std::string pull = "none";
        for (const Two& r : b.parts) {
            if (r.kind != "resistor" || (r.a != sigNet && r.b != sigNet)) { continue; }
            const std::string far = otherEnd(r, sigNet);
            if (b.supply.count(far)) {
                const std::string kind = b.supply.at(far) > 0.0 ? "up" : "down";
                pull = fitted(b, r) ? kind + " (" + r.ref + ")" : "none (" + r.ref + " is DNP)";
            }
        }
        const std::string fw = b.fwPull.count(pin) ? b.fwPull.at(pin) : "none";
        std::cout << "  " << s.ref << " on " << pin << ": pressed connects it to "
                  << (toGround ? "GND, so pressed reads LOW (active-low)" : otherNet)
                  << "; board pull: " << pull << "; firmware pull: " << fw << '\n';
        if (pull.rfind("none", 0) == 0 && fw == "none") {
            std::cout << "      PROBLEM: " << pin << " floats when the button is not pressed\n";
            ++problems;
        }
    }
    return problems;
}

int checkDecoupling(const Board& b, const std::string& vddNet)
{
    int vddPins = 0;
    for (const auto& [pin, net] : b.mcuPin) {
        vddPins += (net == vddNet && pin.rfind("VDD", 0) == 0) ? 1 : 0;
    }
    int caps = 0;
    for (const Two& c : b.parts) {
        if (c.kind == "capacitor" && fitted(b, c) && (c.a == vddNet || c.b == vddNet)) { ++caps; }
    }
    std::cout << "  " << vddPins << " VDD pins on " << b.mcuRef << ", " << caps
              << " fitted capacitors on " << vddNet;
    if (caps < vddPins) {
        std::cout << "  PROBLEM: the datasheet rule asks for one per VDD pin\n";
        return 1;
    }
    std::cout << "  ok\n";
    return 0;
}

}  // namespace

int main()
{
    Board b;
    std::string text;
    while (std::getline(std::cin, text)) {
        if (text.empty() || text[0] == '#') { continue; }
        std::istringstream in(text);
        std::string kw;
        in >> kw;
        if (kw == "board") { std::getline(in >> std::ws, b.name); }
        else if (kw == "supply") { std::string n; double v = 0; in >> n >> v; b.supply[n] = v; }
        else if (kw == "mcu") { in >> b.mcuRef; }
        else if (kw == "pin") { std::string p, n; in >> p >> n; b.mcuPin[p] = n; }
        else if (kw == "limit") { std::string what; in >> what >> b.pinMaxA; }
        else if (kw == "dnp") { std::string r; in >> r; b.dnp.insert(r); }
        else if (kw == "firmware_pull") { std::string p, m; in >> p >> m; b.fwPull[p] = m; }
        else if (kw == "resistor" || kw == "capacitor" || kw == "led" || kw == "button") {
            Two p;
            p.kind = kw;
            in >> p.ref;
            if (kw == "resistor") { in >> p.value; }
            if (kw == "capacitor") { std::string v; in >> v; }
            in >> p.a >> p.b;
            if (kw == "led") { in >> p.value >> p.imax; }
            b.parts.push_back(p);
        } else {
            std::cerr << "unknown keyword: " << kw << '\n';
            return 2;
        }
    }
    double vdd = 0.0;
    std::string vddNet;
    for (const auto& [net, v] : b.supply) {
        if (v > vdd) { vdd = v; vddNet = net; }
    }
    std::cout << "board: " << b.name << "\n" << "supply: " << vddNet << " = " << vdd << " V\n";
    std::cout << "LEDs:\n";
    int problems = checkLeds(b, vdd);
    std::cout << "buttons:\n";
    problems += checkButtons(b);
    std::cout << "decoupling:\n";
    problems += checkDecoupling(b, vddNet);
    std::cout << "problems found: " << problems << '\n';
    return 0;
}
