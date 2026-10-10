// F9-08 Listing 1: from robot requirements to a window of usable gear ratios.
// Robot, wheel and motor values are PRETEND exercise values; a real design takes
// them from the robot's specification and the gearmotor's datasheet.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>

struct Motor  // straight torque-speed line at the rated voltage (F1-69)
{
    double stallTorque;   // N m at zero speed
    double noLoadSpeed;   // rad/s at zero torque
    double ratedTorque;   // N m the motor may deliver continuously without overheating

    double torqueAt(double speed) const
    {
        return stallTorque * (1.0 - speed / noLoadSpeed);
    }
};

int main()
{
    const double g = 9.81;             // m/s^2, rounded; the local value varies slightly
    const double mass = 4.0;           // kg, whole robot
    const double wheelRadius = 0.035;  // m
    const int drivenWheels = 2;
    const double topSpeed = 0.6;       // m/s
    const double accel = 1.0;          // m/s^2, required on flat floor
    const double slopeDeg = 10.0;      // steepest ramp, climbed at top speed
    const double rolling = 0.02;       // rolling-resistance coefficient
    const double gearEff = 0.80;       // gearbox efficiency
    const Motor motor{0.050, 800.0, 0.012};

    const double slope = slopeDeg * std::numbers::pi / 180.0;
    const double fAccel = mass * accel + rolling * mass * g;
    const double fClimb = mass * g * std::sin(slope) + rolling * mass * g * std::cos(slope);
    const double tAccel = fAccel * wheelRadius / drivenWheels;  // per wheel, N m
    const double tClimb = fClimb * wheelRadius / drivenWheels;
    const double wheelSpeed = topSpeed / wheelRadius;           // rad/s

    std::printf("force: accelerate %.3f N, climb %.3f N\n", fAccel, fClimb);
    std::printf("per wheel: accelerate %.4f N m, climb %.4f N m, top speed %.2f rad/s\n",
                tAccel, tClimb, wheelSpeed);
    std::printf("\n ratio  motor_speed  climb_torque  avail_torque  accel_torque  verdict\n");
    for (int ratio : {10, 15, 20, 30, 40, 45, 50, 75}) {
        const double motorSpeed = wheelSpeed * ratio;
        const double needClimb = tClimb / (ratio * gearEff);
        const double needAccel = tAccel / (ratio * gearEff);
        const double avail = motor.torqueAt(motorSpeed);
        const char* verdict = "OK";
        if (motorSpeed >= motor.noLoadSpeed) {
            verdict = "too fast: top speed unreachable";
        } else if (needClimb > avail) {
            verdict = "too weak: stalls on the ramp";
        } else if (needClimb > motor.ratedTorque) {
            verdict = "overheats on a long ramp";
        } else if (needAccel > motor.stallTorque) {
            verdict = "cannot reach the acceleration";
        }
        std::printf("%5d %10.1f %13.5f %13.5f %13.5f   %s\n", ratio, motorSpeed, needClimb, avail,
                    needAccel, verdict);
    }
    return 0;
}
