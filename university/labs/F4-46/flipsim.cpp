// flipsim.cpp - DR405 F4-46: the same rendered frames shown with synchronised and with
// asynchronous page flips. A renderer finishes a new buffer every 11,000 us (faster than
// the 75 Hz refresh of the mode), so some refreshes get a new buffer and some do not.
#include "flipsim.h"

int main()
{
    const flip::Mode m = flip::kQemu640;
    std::printf("mode: %.0f kHz, htotal %d, vtotal %d, vactive %d -> line %.3f us, frame %.1f us (%.2f Hz)\n",
                m.clock_khz, m.htotal, m.vtotal, m.vactive, flip::line_us(m), flip::frame_us(m),
                1e6 / flip::frame_us(m));
    std::vector<double> ready;
    for (int i = 1; i <= 8; ++i) ready.push_back(2500.0 + 11000.0 * i);
    for (bool sync : {true, false}) {
        std::vector<flip::Refresh> rows;
        std::vector<flip::Flip> flips;
        flip::run(m, ready, sync, 8, rows, flips);
        std::printf("\n%s flips:\n", sync ? "synchronised (latched at the start of vertical blank)" : "asynchronous (latched at once)");
        flip::print(m, rows, flips);
    }
    return 0;
}
