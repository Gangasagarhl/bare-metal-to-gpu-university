// F1-78 Listing 1: an executable model of TIM2 of the teaching chip U-MCU-1, written from
// the reference-manual excerpt umcu1_rm_tim2.txt (section numbers in the comments).
// Input: a script of register writes and reads, as firmware would do them.
//   write <register> <value>    read <register>
//   run <cycles>                advance TIM2CLK by that many cycles, report update events
//   blink <count> <clear>       firmware loop: wait for UIF, write <clear> to TIM2_SR, toggle LED
//   pwm <cycles>                measure OC1 (frequency and duty) over that many cycles
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace {

constexpr double kTimClkHz = 72'000'000.0;   // 7.3: TIM2CLK in the lab configuration

class Tim2 {
public:
    std::uint64_t now = 0;   // TIM2CLK cycles since reset
    int updates = 0;         // update events since the last "run"

    void write(const std::string& reg, std::uint32_t v)
    {
        if (reg == "RCC_APB1ENR") { clockOn_ = (v & 1u) != 0; return; }
        if (!clockOn_) {       // 7.6: writes ignored while the clock is off
            std::cout << "  (write to " << reg << " ignored: TIM2EN = 0)\n";
            return;
        }
        v &= 0xFFFFu;
        if (reg == "TIM2_CR1") { cr1_ = v & 0x81u; }
        else if (reg == "TIM2_DIER") { dier_ = v & 1u; }
        else if (reg == "TIM2_SR") { if (v & 1u) { uif_ = false; } }   // 14.4.4: w1c
        else if (reg == "TIM2_EGR") { if (v & 1u) { updateEvent(); } } // 14.4.5
        else if (reg == "TIM2_CCMR1") { ccmr1_ = v & 0x70u; }
        else if (reg == "TIM2_CCER") { ccer_ = v & 1u; }
        else if (reg == "TIM2_CNT") { cnt_ = v; }
        else if (reg == "TIM2_PSC") { pscPreload_ = v; }                // 14.2: buffered
        else if (reg == "TIM2_ARR") { arrPreload_ = v; if (!(cr1_ & 0x80u)) { arr_ = v; } }
        else if (reg == "TIM2_CCR1") { ccr1_ = v; }
        else { std::cout << "  unknown register " << reg << '\n'; }
    }

    std::uint32_t read(const std::string& reg) const
    {
        if (reg == "RCC_APB1ENR") { return clockOn_ ? 1u : 0u; }
        if (!clockOn_) { return 0; }   // 7.6
        if (reg == "TIM2_CR1") { return cr1_; }
        if (reg == "TIM2_SR") { return uif_ ? 1u : 0u; }
        if (reg == "TIM2_CNT") { return cnt_; }
        if (reg == "TIM2_PSC") { return pscPreload_; }
        if (reg == "TIM2_ARR") { return arrPreload_; }
        if (reg == "TIM2_CCR1") { return ccr1_; }
        if (reg == "TIM2_EGR") { return 0; }
        return 0;
    }

    bool uif() const { return uif_; }
    bool oc1() const { return ccmr1_ == 0x60u && ccer_ && cnt_ < ccr1_; }   // 14.3

    // Advance by one TIM2CLK cycle; returns true when an update event happened.
    bool tick()
    {
        ++now;
        if (!clockOn_ || !(cr1_ & 1u)) { return false; }
        if (++prescale_ <= psc_) { return false; }                  // 14.2
        prescale_ = 0;
        if (cnt_ >= arr_) { updateEvent(); return true; }           // 14.1
        ++cnt_;
        return false;
    }

    std::uint32_t activePsc() const { return psc_; }

private:
    void updateEvent()
    {
        cnt_ = 0;
        prescale_ = 0;
        psc_ = pscPreload_;
        if (cr1_ & 0x80u) { arr_ = arrPreload_; }
        uif_ = true;
        ++updates;
    }

    bool clockOn_ = false, uif_ = false;
    std::uint32_t cr1_ = 0, dier_ = 0, ccmr1_ = 0, ccer_ = 0;
    std::uint32_t cnt_ = 0, psc_ = 0, pscPreload_ = 0, prescale_ = 0;
    std::uint32_t arr_ = 0xFFFF, arrPreload_ = 0xFFFF, ccr1_ = 0;
};

double ms(std::uint64_t cycles)
{
    return 1000.0 * static_cast<double>(cycles) / kTimClkHz;
}

std::uint32_t parse(const std::string& s)
{
    return static_cast<std::uint32_t>(std::stoul(s, nullptr, 0));
}

}  // namespace

int main()
{
    Tim2 t;
    std::string text;
    std::cout << std::fixed << std::setprecision(3);
    while (std::getline(std::cin, text)) {
        if (text.empty() || text[0] == '#') {
            if (text.rfind("##", 0) == 0) { std::cout << text << '\n'; }   // titles
            continue;
        }
        std::istringstream in(text);
        std::string cmd, a, b;
        in >> cmd >> a >> b;
        if (cmd == "write") {
            std::cout << "write " << a << " = " << b << '\n';
            t.write(a, parse(b));
        } else if (cmd == "read") {
            std::cout << "read  " << a << " -> 0x" << std::hex << t.read(a) << std::dec << '\n';
        } else if (cmd == "run") {
            const std::uint64_t n = parse(a), start = t.now;
            t.updates = 0;
            int shown = 0;
            for (std::uint64_t i = 0; i < n; ++i) {
                if (t.tick() && shown < 3) {
                    std::cout << "  update event at t = " << ms(t.now) << " ms\n";
                    ++shown;
                }
            }
            std::cout << "run " << ms(n) << " ms (" << n << " cycles): " << t.updates
                      << " update events; active PSC " << t.activePsc() << "; t = "
                      << ms(t.now) << " ms (from " << ms(start) << ")\n";
        } else if (cmd == "blink") {
            const int count = static_cast<int>(parse(a));
            const std::uint32_t clearValue = parse(b);
            std::uint64_t last = t.now;
            std::cout << "blink: wait for UIF, write " << clearValue << " to TIM2_SR, toggle LED\n";
            for (int k = 0; k < count; ++k) {
                std::uint64_t guard = 0;
                while (!t.uif() && guard < 400'000'000) { t.tick(); ++guard; }
                t.write("TIM2_SR", clearValue);
                t.tick();   // the loop itself takes time too: one cycle in this model
                std::cout << "  LED toggle " << (k + 1) << " at t = " << ms(t.now)
                          << " ms (" << ms(t.now - last) << " ms after the previous)\n";
                last = t.now;
            }
        } else if (cmd == "pwm") {
            const std::uint64_t n = parse(a);
            std::uint64_t high = 0, rises = 0;
            bool prev = t.oc1();
            for (std::uint64_t i = 0; i < n; ++i) {
                t.tick();
                const bool o = t.oc1();
                high += o ? 1 : 0;
                rises += (o && !prev) ? 1 : 0;
                prev = o;
            }
            std::cout << "pwm over " << ms(n) << " ms: " << rises << " rising edges ("
                      << std::setprecision(1) << rises / (ms(n) / 1000.0) << " Hz), high "
                      << 100.0 * static_cast<double>(high) / static_cast<double>(n) << " % of the time\n"
                      << std::setprecision(3);
        } else {
            std::cout << "unknown command " << cmd << '\n';
            return 2;
        }
    }
    return 0;
}
