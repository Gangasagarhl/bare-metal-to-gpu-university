// clock_gate.cpp - BR-07 trap 1: the device that "reads zeros" because its clock is off.
//
// A tiny model of three blocks of an imaginary SoC (this course's own model, not a real chip):
//   a clock controller (one gate bit per clock; every gate starts closed),
//   a reset controller (one bit per line; 1 = held in reset; every line starts held),
//   a UART-like block wired to clock 5 and reset line 5.
// The model's rule for a block whose clock is gated or whose reset is held: every register
// reads as 0 and every write is lost. That is ONE behaviour real SoCs show; others hang the
// bus or raise an external abort. Which one your SoC has is in its TRM, if anywhere.
#include <cstdint>
#include <cstdio>

namespace {

constexpr uint32_t kCcuGate = 0x03000000;    // clock controller: gate register
constexpr uint32_t kRcuAssert = 0x03001000;  // reset controller: "held in reset" register
constexpr uint32_t kUartBase = 0x04500000;   // the UART-like block
constexpr uint32_t kUartDiv = kUartBase + 0x24;  // a configuration register (baud divisor)
constexpr uint32_t kUartId = kUartBase + 0xfc;   // an identification register
constexpr int kUartClock = 5;                // wiring fixed in silicon: the devicetree says
constexpr int kUartReset = 5;                // clocks = <&ccu 5>; resets = <&rcu 5>;

struct Soc {
    uint32_t gates = 0;               // bit n = clock n running
    uint32_t held = 0xffffffffu;      // bit n = reset line n held
    uint32_t uart_div = 0;

    bool uart_alive() const
    {
        return (gates >> kUartClock & 1u) != 0 && (held >> kUartReset & 1u) == 0;
    }

    uint32_t read(uint32_t addr) const
    {
        if (addr == kCcuGate) {
            return gates;
        }
        if (addr == kRcuAssert) {
            return held;
        }
        if (addr >= kUartBase && addr < kUartBase + 0x100) {
            if (!uart_alive()) {
                return 0;             // the trap: no error, just zeros
            }
            return addr == kUartId ? 0x00011550u : addr == kUartDiv ? uart_div : 0;
        }
        return 0;
    }

    void write(uint32_t addr, uint32_t value)
    {
        if (addr == kCcuGate) {
            gates = value;
        } else if (addr == kRcuAssert) {
            held = value;
        } else if (addr == kUartDiv && uart_alive()) {
            uart_div = value;         // lost while the block is gated or held in reset
        }
    }
};

void show(const Soc& soc, const char* when)
{
    std::printf("%-34s gate=0x%08x held=0x%08x  UART id=0x%08x div=%u\n", when,
                soc.read(kCcuGate), soc.read(kRcuAssert), soc.read(kUartId), soc.read(kUartDiv));
}

}  // namespace

int main()
{
    Soc soc;
    std::printf("-- the PC habit: configure and probe at once --\n");
    soc.write(kUartDiv, 26);
    show(soc, "after writing div=26, reading id:");
    std::printf("driver says: id 0x%08x is not 0x00011550, \"no device\"\n\n", soc.read(kUartId));

    std::printf("-- the SoC way: dependencies first, in the order the plan says --\n");
    soc.write(kCcuGate, soc.read(kCcuGate) | (1u << kUartClock));
    show(soc, "clock 5 on (reset still held):");
    soc.write(kRcuAssert, soc.read(kRcuAssert) & ~(1u << kUartReset));
    show(soc, "reset 5 released:");
    std::printf("the earlier write of div=26 was lost: div reads %u\n", soc.read(kUartDiv));
    soc.write(kUartDiv, 26);
    show(soc, "div=26 written again:");
    bool ok = soc.read(kUartId) == 0x00011550u && soc.read(kUartDiv) == 26;
    std::printf("%s\n", ok ? "probe ok" : "probe FAILED");
    return ok ? 0 : 1;
}
