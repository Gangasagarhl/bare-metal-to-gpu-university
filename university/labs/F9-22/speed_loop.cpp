// F9-22 Listing 4: the supervised lab program, run here against the simulated kit.
// Safety layers in software: a sign test before closing the loop, a speed limit on
// the setpoint, a setpoint ramp, a stall watchdog and a stop at the end. The physical
// emergency stop is NOT in this program: it must cut motor power in hardware.
#include "../F9-21/pid.hpp"
#include "motor_io.hpp"
#include <cmath>
#include <format>
#include <iostream>

namespace {

constexpr double kPeriod = 0.01;         // s
constexpr double kSpeedLimit = 400.0;    // rad/s, set by the lab plan before the first run
constexpr double kRampPerSecond = 600.0; // rad/s per s

// Drive gently forward and check that the counter moves forward.
bool signTest(MotorIo& io, double countsPerRev)
{
    const std::uint16_t start = io.readEncoder();
    io.setDuty(0.1);
    io.advance(0.2);
    io.setDuty(0.0);
    const double speed = speedFromCounts(io.readEncoder(), start, countsPerRev, 0.2);
    io.advance(0.5); // let it stop
    std::cout << std::format("sign test: duty +0.10 for 0.2 s gave {:+.1f} rad/s on average\n",
                             speed);
    return speed > 5.0;
}

int runLab(SimulatedKit& kit, double requested)
{
    if (!signTest(kit, kit.countsPerRev)) {
        kit.setDuty(0.0);
        std::cout << "STOP: the motor turns the wrong way or not at all. Check wiring; loop not "
                     "closed.\n";
        return 1;
    }
    rb::PidConfig c;
    c.kp = 0.0482; // from identify (Listing 3), V per rad/s
    c.ki = 0.2474; // from identify (Listing 3), V per rad
    c.period = kPeriod;
    c.outMin = -12.0;
    c.outMax = 12.0;
    c.derivativeTau = 0.0;
    rb::Pid pid = rb::Pid::create(c).value();
    const double goal = std::min(requested, kSpeedLimit);
    std::cout << std::format("setpoint asked {:.0f} rad/s, after speed limit {:.0f} rad/s\n",
                             requested, goal);
    std::uint16_t before = kit.readEncoder();
    double setpoint = 0.0;
    double stalledFor = 0.0;
    std::cout << "  t (s)  setpoint  measured  duty   volts\n";
    for (int k = 0; k < 150; ++k) {
        setpoint = std::min(goal, setpoint + kRampPerSecond * kPeriod);
        const std::uint16_t now = kit.readEncoder();
        const double measured = speedFromCounts(now, before, kit.countsPerRev, kPeriod);
        before = now;
        const double volts = pid.update(setpoint, measured);
        const double duty = volts / kit.supplyVolts;
        stalledFor =
            (std::abs(duty) > 0.5 && std::abs(measured) < 5.0) ? stalledFor + kPeriod : 0.0;
        if (stalledFor >= 0.3) {
            kit.setDuty(0.0);
            std::cout << std::format(
                "STOP at t = {:.2f} s: stall watchdog (high duty, no motion)\n", k * kPeriod);
            return 2;
        }
        kit.setDuty(duty);
        if (k % 10 == 0) {
            std::cout << std::format("{:>7.2f} {:>9.1f} {:>9.1f} {:>5.2f} {:>7.2f}\n", k * kPeriod,
                                     setpoint, measured, duty, volts);
        }
        kit.advance(kPeriod);
    }
    kit.setDuty(0.0);
    std::cout << "end of run: duty set to 0\n";
    return 0;
}

} // namespace

int main()
{
    std::cout << "=== run A: correct wiring, setpoint 300 rad/s ===\n";
    SimulatedKit good;
    const int a = runLab(good, 300.0);
    std::cout << "\n=== run B: motor wires swapped ===\n";
    SimulatedKit swapped;
    swapped.wiredBackwards = true;
    const int b = runLab(swapped, 300.0);
    std::cout << "\n=== run C: shaft jammed at t = 0.8 s ===\n";
    SimulatedKit jammed;
    jammed.jamAt = 0.8;
    const int c = runLab(jammed, 300.0);
    std::cout << std::format("\nresults: run A returned {}, run B returned {}, run C returned {}\n",
                             a, b, c);
    return 0;
}
