// F1-43 forensic evidence: one handler in this build forgets its EOI.
// Every device raises its line regularly for 60 ticks; we count what is served.
#include "pic_model.h"

int main()
{
    std::array<Handler, 8> h{};
    for (Handler& x : h) {
        x = {"other", 1};
    }
    h[0] = {"TIMER", 1};
    h[3] = {"NET", 1};
    h[4] = {"UART", 2};
    h[6] = {"DISK", 2};
    std::array<std::uint8_t, 60> raise{};
    for (std::size_t t = 0; t < raise.size(); ++t) {
        if (t % 5 == 0) raise[t] |= 1u << 0;   // TIMER every 5 ticks
        if (t % 7 == 1) raise[t] |= 1u << 3;   // NET
        if (t % 6 == 2) raise[t] |= 1u << 4;   // UART
        if (t % 9 == 4) raise[t] |= 1u << 6;   // DISK
    }
    int raised[8] = {};
    for (std::uint8_t r : raise) {
        for (int l = 0; l < 8; ++l) raised[l] += (r >> l) & 1;
    }
    for (int forget : {-1, 4}) {
        Controller c;
        const auto served = simulate(raise, h, c, forget, false);
        std::printf("build %s\n", forget < 0 ? "A (all handlers send EOI)" : "B (the shipped driver)");
        for (int l : {0, 3, 4, 6}) {
            std::printf("  line %d %-5s raised %2d  served %2d\n", l, h[l].name, raised[l], served[l]);
        }
        std::printf("  at the end: IRR=%s ISR=%s\n", bits(c.irr).c_str(), bits(c.isr).c_str());
    }
    return 0;
}
