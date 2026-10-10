// F9-13 forensic generator: a robot patrols a route with a ramp. Its battery is modelled
// as an open-circuit voltage that falls linearly with the charge used, in series with an
// internal resistance. The battery's protection cuts the output if the terminal voltage
// stays below its cutoff for 0.5 s. The log prints one line per minute.
// ALL VALUES ARE PRETEND; the fault is explained in the answer key.
#include <cstdio>

int main()
{
    const double capacityAh = 5.0;
    const double vFull = 12.6, vEmpty = 10.5;  // open-circuit voltage at full / empty
    const double rInt = 0.10;                  // ohm
    const double cutoff = 10.9;                // V
    const double dt = 0.1;                     // s
    double usedAh = 0.0;
    double below = 0.0;                        // time spent below the cutoff, s
    double minV = 99.0, maxI = 0.0;
    std::printf(" min  used_Ah  avg_A  min_terminal_V  open_circuit_V  max_A  state\n");
    double sumI = 0.0;
    int n = 0;
    for (int k = 1;; ++k) {
        const double t = k * dt;
        const double phase = t - 60.0 * static_cast<int>(t / 60.0);  // a 60 s patrol loop
        double current = 3.0;              // computer, sensors and level driving
        if (phase >= 20.0 && phase < 28.0) {
            current = 9.0;                 // climbing the ramp
        }
        usedAh += current * dt / 3600.0;
        const double ocv = vFull - (vFull - vEmpty) * usedAh / capacityAh;
        const double vt = ocv - current * rInt;
        sumI += current;
        ++n;
        if (vt < minV) minV = vt;
        if (current > maxI) maxI = current;
        below = (vt < cutoff) ? below + dt : 0.0;
        const bool cut = below >= 0.5;
        if (k % 600 == 0 || cut) {
            std::printf("%4.1f %8.3f %6.2f %15.2f %15.2f %6.1f  %s\n", t / 60.0, usedAh, sumI / n,
                        minV, ocv, maxI, cut ? "PROTECTION CUT OFF" : "running");
            sumI = 0.0;
            n = 0;
            minV = 99.0;
            maxI = 0.0;
        }
        if (cut) {
            // after the cut the current is zero; the terminal voltage recovers to the OCV
            std::printf("after cut-off: terminal voltage at rest %.2f V (open-circuit), "
                        "used %.2f of %.1f Ah\n",
                        ocv, usedAh, capacityAh);
            break;
        }
    }
    return 0;
}
