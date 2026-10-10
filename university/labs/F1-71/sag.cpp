// F1-71 Listing 2: a PRETEND cell under load: open-circuit voltage (OCV) that
// falls as charge is used, plus an internal resistance. Terminal voltage
// V = OCV(state of charge) - I * R_internal. Discharge stops at a pretend cutoff.
// Every number is an exercise value, NOT data for any real cell or chemistry.
#include <array>
#include <cstdio>
#include <initializer_list>

// OCV table at state of charge 0 %, 10 %, ..., 100 % (pretend values).
constexpr std::array<double, 11> kOcv = {3.00, 3.30, 3.42, 3.50, 3.55, 3.60,
                                         3.65, 3.72, 3.80, 3.90, 4.00};

double ocv(double soc)
{
    if (soc <= 0.0) {
        return kOcv[0];
    }
    if (soc >= 1.0) {
        return kOcv[10];
    }
    const double x = soc * 10.0;
    const int i = static_cast<int>(x);
    return kOcv[i] + (kOcv[i + 1] - kOcv[i]) * (x - i);
}

int main()
{
    const double capacityAh = 2.0;
    const double rInt = 0.05;    // ohm
    const double cutoff = 3.20;  // V
    const double dt = 1.0;       // s
    std::printf("full cell at rest: %.2f V\n\n", ocv(1.0));
    std::printf("%7s %12s %12s %12s %14s\n", "load A", "V at start", "time to cut", "Ah given",
                "heat in cell");
    for (double amps : {1.0, 5.0, 10.0, 20.0}) {
        double soc = 1.0;
        double t = 0.0;
        double heatJ = 0.0;
        const double vStart = ocv(soc) - amps * rInt;
        while (ocv(soc) - amps * rInt > cutoff && soc > 0.0) {
            soc -= amps * dt / 3600.0 / capacityAh;
            heatJ += amps * amps * rInt * dt;
            t += dt;
        }
        std::printf("%7.1f %10.2f V %9.1f min %10.2f Ah %11.0f J (%.2f W)\n", amps, vStart,
                    t / 60.0, (1.0 - soc) * capacityAh, heatJ, amps * amps * rInt);
    }
    return 0;
}
