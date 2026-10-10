// F9-24 Listing 2: verify odometry against simulator ground truth.
// The simulator integrates the true motion with 10-microsecond steps; the odometry
// reads the wheel angles at a fixed rate and integrates with Euler or with the exact arc.
#include "diffdrive.hpp"
#include <cmath>
#include <cstdio>

const double r = 0.05, W = 0.30; // simulated course robot: wheel radius, track width (m)

Wheels profile(double t) // the wheel-speed commands of the test drive (rad/s)
{
    return {8.0 + 4.0 * std::sin(0.5 * t), 8.0 + 4.0 * std::cos(0.3 * t)};
}

int main()
{
    const double T = 10.0, hSim = 1e-5;
    const int nSim = static_cast<int>(std::lround(T / hSim));
    const double rates[] = {100.0, 50.0, 10.0, 2.0}; // odometry rates (Hz)

    std::printf("odometry rate  method  final x   final y   final th   pos error   th error\n");
    for (double hz : rates) {
        const int every = static_cast<int>(std::lround(1.0 / (hz * hSim)));
        const double dt = every * hSim;
        Pose truth, euler, arc;
        double angL = 0, angR = 0, lastL = 0, lastR = 0; // wheel angles (rad)
        for (int k = 0; k < nSim; ++k) {
            const Wheels q = profile(k * hSim);
            truth = stepArc(truth, forwardKin(q, r, W), hSim); // ground truth
            angL += q.left * hSim;
            angR += q.right * hSim;
            if ((k + 1) % every == 0) { // odometry tick: average wheel speed since last tick
                const Wheels avg{(angL - lastL) / dt, (angR - lastR) / dt};
                lastL = angL;
                lastR = angR;
                euler = stepEuler(euler, forwardKin(avg, r, W), dt);
                arc = stepArc(arc, forwardKin(avg, r, W), dt);
            }
        }
        for (int m = 0; m < 2; ++m) {
            const Pose& p = m == 0 ? euler : arc;
            std::printf("%6.0f Hz      %-6s %8.4f  %8.4f  %8.4f   %9.2e  %9.2e\n", hz,
                        m == 0 ? "Euler" : "arc", p.x, p.y, p.theta,
                        std::hypot(p.x - truth.x, p.y - truth.y), std::abs(p.theta - truth.theta));
        }
        if (hz == rates[3]) {
            std::printf("ground truth           %8.4f  %8.4f  %8.4f\n", truth.x, truth.y,
                        truth.theta);
        }
    }
    return 0;
}
