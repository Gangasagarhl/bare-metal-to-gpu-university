// F1-68 Listing 2: from light to a pixel number.
// A pretend pixel collects electrons in proportion to light x exposure time, holds
// at most 10000 electrons (full well), and a 10-bit ADC turns the charge into a code.
// Photon shot noise is modelled as a standard deviation of sqrt(N) electrons.
// Every number here is a pretend exercise value, not a real sensor's.
#include <cmath>
#include <cstdio>
#include <initializer_list>

int main()
{
    const double fullWell = 10000.0;  // electrons (pretend)
    const int maxCode = 1023;          // 10-bit ADC
    std::printf("%12s %10s %10s %8s %10s %8s\n", "light e-/ms", "exposure", "electrons",
                "code", "shot sd e-", "SNR");
    for (double light : {2.0, 50.0, 1000.0}) {
        for (double ms : {1.0, 10.0, 40.0}) {
            double e = light * ms;
            const bool saturated = e > fullWell;
            if (saturated) {
                e = fullWell;
            }
            const int code = static_cast<int>(std::floor(e / fullWell * maxCode));
            const double sd = std::sqrt(e);
            std::printf("%12.0f %8.0f ms %10.0f %8d %10.1f %8.1f%s\n", light, ms, e, code, sd,
                        e / sd, saturated ? "  SATURATED" : "");
        }
    }
    std::printf("\none code step = %.2f electrons\n", fullWell / maxCode);
    return 0;
}
