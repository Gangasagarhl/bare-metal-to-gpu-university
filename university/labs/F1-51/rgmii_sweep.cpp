// rgmii_sweep.cpp - F1-51 forensic answer-key run: receive 100 frames for every total
// clock delay from 0 to 7 ticks and print which settings work in the toy model.
#include "rgmii_model.h"

#include <cstdio>

int main()
{
    std::printf("total delay (ticks)  margin to nearest data change  good frames of 100\n");
    for (int d = 0; d < 2 * toy::kHalfPeriod; ++d) {
        const int good = toy::receiveGood(d, 100, 11);
        std::printf("%10d %24d %26d%s\n", d, toy::margin(d), good,
                    d == 2 ? "   <- exactly one delay switch on" : (d == 4 ? "   <- two delay switches on" : ""));
    }
    std::printf("PHY on + MAC off = 2 ticks; PHY on + MAC on = 4 ticks; both off = 0 ticks\n");
    return 0;
}
