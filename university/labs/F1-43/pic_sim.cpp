// F1-43 Listing 1: priorities, masking, nesting and EOI in the toy controller.
#include "pic_model.h"

int main()
{
    std::array<Handler, 8> h{};
    for (Handler& x : h) {
        x = {"other", 2};
    }
    h[0] = {"TIMER", 2};
    h[4] = {"UART", 4};
    h[5] = {"AUDIO", 2};
    h[6] = {"DISK", 3};
    Controller c;
    c.imr = 0b0010'0000;               // line 5 (AUDIO) is masked by the driver
    std::array<std::uint8_t, 16> raise{};
    raise[0] = 1u << 4;                // UART
    raise[2] = 1u << 0;                // TIMER arrives while UART runs
    raise[3] = (1u << 6) | (1u << 5);  // DISK and the masked AUDIO
    simulate(raise, h, c, -1, true);
    std::printf("end: IRR=%s (still pending) IMR=%s ISR=%s\n", bits(c.irr).c_str(),
                bits(c.imr).c_str(), bits(c.isr).c_str());
    c.imr = 0;                         // the driver unmasks line 5
    std::array<std::uint8_t, 4> none{};
    simulate(none, h, c, -1, true);
    return 0;
}
