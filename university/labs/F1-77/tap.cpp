// F1-77 Listing 1: a model of a JTAG test access port (TAP) controller and of a made-up
// teaching device behind it (4-bit instruction register, IDCODE and BYPASS registers).
// The 16-state machine follows the author's reading of IEEE 1149.1; it was NOT checked
// against the standard in this build (see the unverified box in F1-77). The device's
// instruction codes and its IDCODE value are our own inventions, not a real chip's.
#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

namespace {

enum State {
    TLR, RTI, SelDR, CapDR, ShDR, Ex1DR, PauDR, Ex2DR, UpdDR,
    SelIR, CapIR, ShIR, Ex1IR, PauIR, Ex2IR, UpdIR
};

const std::array<const char*, 16> kName = {
    "Test-Logic-Reset", "Run-Test/Idle", "Select-DR-Scan", "Capture-DR", "Shift-DR",
    "Exit1-DR", "Pause-DR", "Exit2-DR", "Update-DR", "Select-IR-Scan", "Capture-IR",
    "Shift-IR", "Exit1-IR", "Pause-IR", "Exit2-IR", "Update-IR"};

// next[state][tms]
const std::array<std::array<State, 2>, 16> kNext = {{
    {RTI, TLR},     {RTI, SelDR},   {CapDR, SelIR}, {ShDR, Ex1DR},
    {ShDR, Ex1DR},  {PauDR, UpdDR}, {PauDR, Ex2DR}, {ShDR, UpdDR},
    {RTI, SelDR},   {CapIR, TLR},   {ShIR, Ex1IR},  {ShIR, Ex1IR},
    {PauIR, UpdIR}, {PauIR, Ex2IR}, {ShIR, UpdIR},  {RTI, SelDR}}};

constexpr unsigned kIrBits = 4;
constexpr std::uint32_t kIdcodeInstr = 0b1110;   // our device's IDCODE instruction
constexpr std::uint32_t kBypassInstr = 0b1111;   // all ones selects BYPASS
constexpr std::uint32_t kIdcode = 0x0EDC0C01;    // made-up value; bit 0 is 1

struct Tap {
    State state = TLR;
    std::uint32_t ir = kIdcodeInstr;   // the reset value selects IDCODE
    std::uint64_t shift = 0;           // the shift register currently between TDI and TDO
    unsigned shiftLen = 0;
    unsigned tcks = 0;

    unsigned drLength() const { return ir == kIdcodeInstr ? 32 : 1; }

    // One TCK rising edge: returns the bit seen on TDO in a shift state, or -1.
    int clock(int tms, int tdi)
    {
        int tdo = -1;
        if ((state == ShDR || state == ShIR) && shiftLen > 0) {
            tdo = static_cast<int>(shift & 1u);
            shift = (shift >> 1) | (static_cast<std::uint64_t>(tdi & 1) << (shiftLen - 1));
        }
        state = kNext[state][tms & 1];
        ++tcks;
        if (state == TLR) { ir = kIdcodeInstr; }
        if (state == CapIR) { shift = 0b0001; shiftLen = kIrBits; }       // captures ...01
        if (state == CapDR) {
            shiftLen = drLength();
            shift = (ir == kIdcodeInstr) ? kIdcode : 0;                   // BYPASS captures 0
        }
        if (state == UpdIR) { ir = static_cast<std::uint32_t>(shift & 0xFu); }
        return tdo;
    }
};

std::string bitsLsbFirst(std::uint64_t v, unsigned n)
{
    std::string s;
    for (unsigned i = 0; i < n; ++i) {
        s += ((v >> i) & 1u) ? '1' : '0';
    }
    return s;
}

// Move from Run-Test/Idle into Shift-xR, shift n bits (TMS=1 on the last), back to idle.
std::uint64_t scan(Tap& t, bool ir, std::uint64_t in, unsigned n)
{
    t.clock(1, 0);                 // RTI -> Select-DR-Scan
    if (ir) { t.clock(1, 0); }     // -> Select-IR-Scan
    t.clock(0, 0);                 // -> Capture
    t.clock(0, 0);                 // -> Shift
    std::uint64_t out = 0;
    for (unsigned i = 0; i < n; ++i) {
        const int tms = (i == n - 1) ? 1 : 0;   // leave Shift on the last bit
        const int tdo = t.clock(tms, static_cast<int>((in >> i) & 1u));
        out |= static_cast<std::uint64_t>(tdo) << i;
    }
    t.clock(1, 0);                 // Exit1 -> Update
    t.clock(0, 0);                 // Update -> Run-Test/Idle
    return out;
}

}  // namespace

int main()
{
    // 1. Property check: five TCKs with TMS=1 reach Test-Logic-Reset from every state.
    int bad = 0;
    for (int s = 0; s < 16; ++s) {
        Tap t;
        t.state = static_cast<State>(s);
        for (int i = 0; i < 5; ++i) {
            t.clock(1, 0);
        }
        if (t.state != TLR) {
            std::cout << "from " << kName[s] << " five TMS=1 clocks end in " << kName[t.state] << '\n';
            ++bad;
        }
    }
    std::cout << "reset check: " << (16 - bad) << " of 16 states reach Test-Logic-Reset with TMS=11111\n";

    // 2. The walk of a debug probe reading the IDCODE, state by state.
    Tap t;
    const std::string tms = "0" "1" "0" "0";   // to Run-Test/Idle, Select-DR, Capture-DR, Shift-DR
    std::cout << "\nwalk: TMS bits " << tms << '\n';
    for (char c : tms) {
        t.clock(c - '0', 0);
        std::cout << "  TMS=" << c << " -> " << kName[t.state] << '\n';
    }
    std::uint64_t id = 0;
    for (unsigned i = 0; i < 32; ++i) {
        const int tdo = t.clock(i == 31 ? 1 : 0, 0);
        id |= static_cast<std::uint64_t>(tdo) << i;
    }
    t.clock(1, 0);
    t.clock(0, 0);
    std::cout << "  shifted 32 bits out of TDO; now in " << kName[t.state] << '\n';
    std::cout << "  IDCODE read = 0x" << std::hex << id << std::dec << " (bit 0 = " << (id & 1u)
              << "), TCK cycles so far: " << t.tcks << '\n';

    // 3. Select BYPASS through the instruction register, then send a pattern through it.
    const std::uint64_t captured = scan(t, true, kBypassInstr, kIrBits);
    std::cout << "\nIR scan of 1111 (BYPASS): captured IR bits (LSB first) "
              << bitsLsbFirst(captured, kIrBits) << ", IR now " << bitsLsbFirst(t.ir, kIrBits) << '\n';
    const std::uint64_t pattern = 0b1011;
    const std::uint64_t back = scan(t, false, pattern, 5);
    std::cout << "DR scan of 5 bits through BYPASS: sent (LSB first) " << bitsLsbFirst(pattern, 5)
              << ", received " << bitsLsbFirst(back, 5) << "  (one-bit delay, first bit 0)\n";
    std::cout << "total TCK cycles: " << t.tcks << '\n';
    return bad == 0 ? 0 : 1;
}
