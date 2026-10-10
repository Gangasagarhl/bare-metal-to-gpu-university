// thermal.cpp - a thermal zone as a model: one temperature, heated by the CPU's power and cooled
// through a thermal resistance to the air; a passive trip point that lowers the frequency step
// by step, and a critical trip point that starts an orderly shutdown. All constants are INVENTED
// TEACHING VALUES (no processor's datasheet was opened); the shape of the policy is the lesson.
// Two runs: normal cooling, then the same load with the fan failed (resistance four times higher).
#include <cstdio>

struct Zone { double temp_c, ambient_c, r_c_per_w, c_j_per_c; };
constexpr double kPassiveC = 80, kHysteresisC = 5, kCriticalC = 95;
static const double kFreqSteps[] = {3.0, 2.5, 2.0, 1.5, 1.0};   // GHz levels (model)
constexpr int kSteps = 5;
static double power_w(double ghz) { return 5 + 12 * ghz * ghz / 3; }   // model: power grows faster than frequency

static int run(const char* label, double r)
{
    Zone z{40, 25, r, 30};
    int step = 0;
    std::printf("== %s (thermal resistance %.1f C/W) ==\n", label, r);
    for (int s = 0; s <= 600; ++s) {                  // one loop = one second of model time
        const double p = power_w(kFreqSteps[step]);
        z.temp_c += (p - (z.temp_c - z.ambient_c) / z.r_c_per_w) / z.c_j_per_c;
        if (z.temp_c >= kCriticalC) {
            std::printf("  t=%3d s  %.1f C >= critical %.0f C: orderly shutdown: stop new work, "
                        "write caches back, power off\n", s, z.temp_c, kCriticalC);
            return 1;
        }
        const int before = step;
        if (z.temp_c >= kPassiveC && step < kSteps - 1 && s % 5 == 0) ++step;            // passive: slow down
        if (z.temp_c < kPassiveC - kHysteresisC && step > 0 && s % 5 == 0) --step;         // cooled: speed up
        if (step != before || s % 100 == 0)
            std::printf("  t=%3d s  %5.1f C  %.1f GHz  %4.1f W%s\n", s, z.temp_c, kFreqSteps[step], power_w(kFreqSteps[step]),
                        step > before ? "  (passive trip: one step down)" : step < before ? "  (below trip - hysteresis: one step up)" : "");
    }
    std::printf("  600 s: no critical trip; final %.1f C at %.1f GHz\n", z.temp_c, kFreqSteps[step]);
    return 0;
}

int main()
{
    const int a = run("normal cooling", 2.0);
    const int b = run("fan failed", 8.0);
    std::printf("normal: %s; fan failed: %s\n", a ? "shut down" : "kept running", b ? "shut down" : "kept running");
    return 0;
}
