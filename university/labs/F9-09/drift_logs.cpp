// F9-09 forensic generator: two robots, A and B, each drive 4 m along a taped straight
// line (ground truth: perfectly straight, checked by the operator). Both robots' odometry
// says they drifted left. This program writes their encoder logs as 0.25 s intervals.
// The faults injected are described in the answer key only. ALL VALUES ARE PRETEND.
#include <cmath>
#include <cstdio>
#include <numbers>

struct Fault
{
    double leftDiameterTrue;   // m, real rolling diameter of the left wheel
    double rightDiameterTrue;  // m
    double slipAccel;          // extra spin of the right wheel while accelerating (fraction)
    double slipPatch;          // extra spin of the right wheel on the polished patch (fraction)
};

void run(const char* name, const Fault& f)
{
    const double paramDiameter = 0.070;  // m, the value in the robot's configuration
    const double track = 0.160;
    const double cpr = 1440.0;
    const double dt = 0.25;
    double s = 0.0;  // true distance along the line, m
    double v = 0.0;
    double cL = 0.0;  // counts as real numbers; the logged value is rounded
    double cR = 0.0;
    long loggedL = 0;
    long loggedR = 0;
    double heading = 0.0;  // odometry heading, rad
    double y = 0.0;        // odometry lateral offset, m
    std::printf("robot %s\n  t_s   dist_m  dL_counts  dR_counts  dL/dR   odo_heading_deg\n", name);
    for (int k = 1; s < 4.0; ++k) {
        const double t = k * dt;
        double a = 0.0;
        if (t <= 1.0) {
            a = 0.5;  // m/s^2 up to 0.5 m/s
        } else if (s > 3.75) {
            a = -0.5;
        }
        v = std::fmax(v + a * dt, 0.05);
        const double ds = v * dt;
        s += ds;
        double slip = 0.0;
        if (a > 0.0) {
            slip = f.slipAccel;
        }
        if (s > 2.0 && s < 2.6) {
            slip = f.slipPatch;
        }
        cL += ds / (std::numbers::pi * f.leftDiameterTrue) * cpr;
        cR += ds * (1.0 + slip) / (std::numbers::pi * f.rightDiameterTrue) * cpr;
        const long nowL = std::lround(cL);
        const long nowR = std::lround(cR);
        const long dL = nowL - loggedL;
        const long dR = nowR - loggedR;
        loggedL = nowL;
        loggedR = nowR;
        const double mPerCount = std::numbers::pi * paramDiameter / cpr;
        const double dsL = dL * mPerCount;
        const double dsR = dR * mPerCount;
        const double dth = (dsR - dsL) / track;
        y += (dsL + dsR) / 2.0 * std::sin(heading + dth / 2.0);
        heading += dth;
        std::printf("%5.2f %8.3f %10ld %10ld %7.4f %12.2f\n", t, s, dL, dR,
                    static_cast<double>(dL) / static_cast<double>(dR),
                    heading * 180.0 / std::numbers::pi);
    }
    std::printf("  total counts L %ld R %ld; odometry: heading %.2f deg, lateral offset %.3f m "
                "to the left\n\n",
                loggedL, loggedR, heading * 180.0 / std::numbers::pi, y);
}

int main()
{
    run("A", Fault{0.0714, 0.070, 0.0, 0.0});
    run("B", Fault{0.070, 0.070, 0.04, 0.06});
    return 0;
}
