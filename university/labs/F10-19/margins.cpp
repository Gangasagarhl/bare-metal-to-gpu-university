// margins.cpp - how much rate-loop gain the course quad tolerates, and how delay eats it.
// Roll-rate loop alone (P only: I = 0, D = 0), DN201 simulator, controller at 500 Hz.
// The controller sees the state 'delay' ms late. For each delay, bisection finds the
// smallest P at which a small kick no longer dies out (the "ultimate gain" Ku: the late
// peak is at least half the early peak; motor limits stop the growth of an unstable loop)
// and the frequency of the oscillation there. Gain margin of the course P (20) = Ku / 20.
#include "../F10-16/cascade.hpp"

#include <cmath>
#include <cstdio>
#include <deque>

namespace {

struct Response
{
    double early = 0.0, late = 0.0; // peak |roll rate| in 1.0-1.5 s and in 2.5-3.0 s
    int crossings = 0;              // sign changes of the roll rate in 2.0-3.0 s
};

Response kick(double kp, int delayMs)
{
    const dn::Params P;
    dn301::Gains g;
    g.rateP = kp;
    g.rateI = 0.0;
    g.rateD = 0.0;
    dn301::Cascade c(P, g);
    c.holdOuter = c.holdAttitude = true;
    c.trace.thrust = P.mass * P.g;
    dn::State s;
    const double w0 = dn::hoverRotorSpeed(P);
    s.rotor = {w0, w0, w0, w0};
    std::deque<dn::State> line;
    Response r;
    double prev = 0.0;
    for (int k = 1; k <= 3000; ++k) {
        c.trace.rateSp = {(k >= 100 && k < 120) ? 0.05 : 0.0, 0.0, 0.0}; // a 20 ms kick
        line.push_back(s);
        const dn::State seen = line.front();
        if (static_cast<int>(line.size()) > delayMs) line.pop_front();
        dn::rk4Step(P, s, c.step(seen, {}, k), 0.001);
        const double p = s.w.x;
        if (k > 1000 && k <= 1500) r.early = std::max(r.early, std::abs(p));
        if (k > 2500) r.late = std::max(r.late, std::abs(p));
        if (k > 2000 && (p > 0.0) != (prev > 0.0)) ++r.crossings;
        prev = p;
    }
    return r;
}

} // namespace

int main()
{
    std::printf("Roll-rate loop, P only. Ku = smallest P whose kick response does not die out.\n");
    std::printf("%10s %10s %14s %18s\n", "delay (ms)", "Ku", "freq at Ku (Hz)", "margin of P = 20");
    for (int delay : {0, 2, 4, 6, 10}) {
        double lo = 10.0, hi = 3000.0;
        for (int i = 0; i < 30; ++i) {
            const double mid = std::sqrt(lo * hi); // bisection on a log scale
            const Response r = kick(mid, delay);
            (r.late >= 0.5 * r.early ? hi : lo) = mid;
        }
        const Response r = kick(hi, delay);
        std::printf("%10d %10.1f %14.1f %15.1f x\n", delay, hi, r.crossings / 2.0, hi / 20.0);
    }
    return 0;
}
