// F0-79 Listing 1: one PI speed controller, three ways to discretise its integral, five sample
// periods. Plant model (lab numbers, not a real motor): tau w' = -w + K u. Simulated with fine RK4
// steps; the controller samples w every T seconds and holds u constant until the next sample
// (zero-order hold).
#include <cmath>
#include <cstdio>

constexpr double kTau = 0.5;  // s
constexpr double kK = 2.0;    // (rad/s) per volt
constexpr double kKp = 2.0;   // V per (rad/s)
constexpr double kKi = 8.0;   // V per rad
constexpr double kH = 1.0e-4; // s: fine step of the plant simulation
constexpr double kEnd = 2.0;  // s

double plantRhs(double w, double u)
{
    return (-w + kK * u) / kTau;
}

double plantStep(double w, double u, double h) // RK4 with u held constant
{
    const double a = plantRhs(w, u), b = plantRhs(w + h / 2 * a, u);
    const double c = plantRhs(w + h / 2 * b, u), d = plantRhs(w + h * c, u);
    return w + h / 6 * (a + 2 * b + 2 * c + d);
}

enum class Rule { Forward, Backward, Tustin, Continuous };

struct Result
{
    double overshoot; // percent above the set point
    double settle;    // last time |w - r| > 2 % of r
    double maxDiff;   // largest |w(t) - w_continuous(t)| on the fine time grid
};

// The continuous-time PI loop, used as the reference (integral state stepped with the plant).
void continuousLoop(double* out, int steps)
{
    double w = 0.0, integ = 0.0;
    const double r = 1.0;
    for (int i = 0; i < steps; ++i) {
        out[i] = w;
        for (int k = 0; k < 4; ++k) { // 25 us sub-steps for the reference loop
            const double e = r - w;
            const double u = kKp * e + kKi * integ;
            w = plantStep(w, u, kH / 4);
            integ += (kH / 4) * e;
        }
    }
}

Result sampledLoop(Rule rule, double T, const double* ref, int steps)
{
    const double r = 1.0;
    double w = 0.0, integ = 0.0, ePrev = 0.0, u = 0.0; // before t = 0 the error was 0
    const int perSample = static_cast<int>(T / kH + 0.5);
    Result res{0.0, 0.0, 0.0};
    for (int i = 0; i < steps; ++i) {
        if (i % perSample == 0) { // sample, compute, hold
            const double e = r - w;
            if (rule == Rule::Forward) { // I[k] uses errors up to k-1
                u = kKp * e + kKi * integ;
                integ += T * e;
            } else if (rule == Rule::Backward) { // I[k] includes e[k]
                integ += T * e;
                u = kKp * e + kKi * integ;
            } else { // trapezoid: average of e[k-1] and e[k]
                integ += T * 0.5 * (e + ePrev);
                u = kKp * e + kKi * integ;
            }
            ePrev = e;
        }
        res.overshoot = std::fmax(res.overshoot, (w - r) / r * 100.0);
        if (std::fabs(w - r) > 0.02 * r) {
            res.settle = i * kH;
        }
        res.maxDiff = std::fmax(res.maxDiff, std::fabs(w - ref[i]));
        w = plantStep(w, u, kH);
    }
    return res;
}

int main()
{
    const int steps = static_cast<int>(kEnd / kH + 0.5);
    static double ref[20001];
    continuousLoop(ref, steps);
    std::printf(
        "plant tau = %.2f s, K = %.1f; PI Kp = %.1f, Ki = %.1f; unit step in speed set point\n",
        kTau, kK, kKp, kKi);
    std::printf("%-8s %-9s %-11s %-10s %-11s\n", "T s", "rule", "overshoot%", "settle s",
                "max|w-wc|");
    Result c{0.0, 0.0, 0.0};
    for (int i = 0; i < steps; ++i) {
        c.overshoot = std::fmax(c.overshoot, (ref[i] - 1.0) * 100.0);
        if (std::fabs(ref[i] - 1.0) > 0.02) {
            c.settle = i * kH;
        }
    }
    std::printf("%-8s %-9s %-11.2f %-10.4f %-11s\n", "-", "contin.", c.overshoot, c.settle, "0");
    const double periods[] = {0.001, 0.01, 0.05, 0.1, 0.2};
    const char* names[] = {"forward", "backward", "Tustin"};
    for (double T : periods) {
        for (int k = 0; k < 3; ++k) {
            const Result r = sampledLoop(static_cast<Rule>(k), T, ref, steps);
            std::printf("%-8.3f %-9s %-11.2f %-10.4f %-11.4f\n", T, names[k], r.overshoot, r.settle,
                        r.maxDiff);
        }
    }
    return 0;
}
