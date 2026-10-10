// F1-70 Listing 3: what an inductor does when its current is switched off.
// A motor winding (pretend L = 1 mH, R = 2 ohm) carries 1 A from a 6 V supply
// through a low-side switch. At t = 0 the switch opens.
// Case A: a freewheeling diode across the winding (pretend 0.7 V drop) gives the
//         current a path; the switch sees supply + 0.7 V.
// Case B: no diode; the open switch is modelled as 10 kilo-ohm (pretend), so the
//         current is forced through it and the voltage jumps.
#include <cstdio>

int main()
{
    const double l = 1.0e-3;
    const double r = 2.0;
    const double supply = 6.0;
    const double vd = 0.7;
    const double rOff = 10000.0;
    const double dt = 1.0e-9;
    double ia = 1.0;
    double ib = 1.0;
    double peakB = 0.0;
    std::printf("%10s %12s %14s %12s %14s\n", "t us", "A: i (A)", "A: switch V", "B: i (A)",
                "B: switch V");
    for (long k = 0; k <= 2000000; ++k) {
        const double t = k * dt;
        const double vSwitchA = ia > 0.0 ? supply + vd : supply;
        const double vSwitchB = ib * rOff;
        peakB = vSwitchB > peakB ? vSwitchB : peakB;
        if (k == 0 || k == 100 || k == 1000 || k == 10000 || k == 100000 || k == 500000
            || k == 2000000) {
            std::printf("%10.3f %12.4f %14.2f %12.6f %14.1f\n", t * 1e6, ia, vSwitchA, ib,
                        vSwitchB);
        }
        // A: inductor drives current around the loop through the diode.
        if (ia > 0.0) {
            ia += -(r * ia + vd) / l * dt;
            ia = ia < 0.0 ? 0.0 : ia;
        }
        // B: the loop is supply, winding and the open switch in series.
        ib += (supply - r * ib - rOff * ib) / l * dt;
    }
    std::printf("peak switch voltage, case B: %.0f V (case A never exceeds %.1f V)\n", peakB,
                supply + vd);
    return 0;
}
