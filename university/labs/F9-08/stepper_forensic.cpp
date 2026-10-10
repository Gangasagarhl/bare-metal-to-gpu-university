// F9-08 forensic generator: a stepper-driven pick-and-place axis moves out 4000 steps
// empty, picks up a part (on some cycles), moves back 4000 steps and checks its home
// switch. The pull-out torque falls linearly with step rate
// (a simplified curve); if the torque needed at a step exceeds what the motor can give,
// the rotor stalls and every later step of that move is lost. ALL VALUES ARE PRETEND.
#include <cmath>
#include <cstdio>
#include <numbers>

struct Axis
{
    double holdTorque = 0.40;   // N m at zero step rate
    double maxRate = 3000.0;    // steps/s at which the available torque reaches zero
    double friction = 0.03;     // N m
    double inertia = 1.0e-4;    // kg m^2, axis without payload
};

// Returns the number of steps lost in one trapezoidal move of `steps` steps.
int move(const Axis& ax, int steps, double accel, double cruise)
{
    const double radPerStep = 2.0 * std::numbers::pi / 200.0;
    double rate = 0.0;
    double pos = 0.0;
    const double decelStart = steps - (cruise * cruise) / (2.0 * accel);
    for (int k = 0; k < steps; ++k) {
        double a = 0.0;
        if (pos < decelStart && rate < cruise) {
            a = accel;
        } else if (pos >= decelStart) {
            a = -accel;
        }
        const double need = ax.inertia * std::fabs(a) * radPerStep + ax.friction;
        const double avail = ax.holdTorque * (1.0 - rate / ax.maxRate);
        if (need > avail) {
            return steps - k;  // stalled: the remaining steps are not followed
        }
        const double next = std::sqrt(std::fmax(rate * rate + 2.0 * a, 1.0));
        rate = std::fmin(next, cruise);
        pos += 1.0;
    }
    return 0;
}

int main()
{
    std::printf("cycle  firmware  accel(steps/s^2)  cruise(steps/s)  part_carried_back"
                "  home_error(steps)\n");
    struct Run
    {
        const char* fw;
        double accel;
        bool payload;
    };
    const Run runs[] = {{"v1.2", 2000, false}, {"v1.2", 2000, true},  {"v1.2", 2000, true},
                        {"v1.3", 6000, false}, {"v1.3", 6000, false}, {"v1.3", 6000, true},
                        {"v1.3", 6000, false}, {"v1.3", 6000, true}};
    int cycle = 1;
    for (const Run& r : runs) {
        const Axis empty;
        Axis back;
        if (r.payload) {
            back.inertia = 5.0e-4;
            back.friction = 0.05;
        }
        const int lostOut = move(empty, 4000, r.accel, 2000.0);
        const int lostBack = move(back, 4000, r.accel, 2000.0);
        // Lost steps on the way out leave the carriage short; on the way back, short of home.
        const int homeError = lostBack - lostOut;
        std::printf("%5d  %8s %17.0f %16.0f %18s %18d\n", cycle, r.fw, r.accel, 2000.0,
                    r.payload ? "yes" : "no", homeError);
        ++cycle;
    }
    return 0;
}
