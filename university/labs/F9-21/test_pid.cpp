// F9-21 Listing 2: unit tests for pid.hpp. Each test prints PASS or FAIL;
// the program returns 1 if any test fails, so a build script can stop on it.
#include "pid.hpp"
#include <cmath>
#include <format>
#include <iostream>
#include <limits>
#include <string>

namespace {

int failures = 0;

void check(bool ok, const std::string& name)
{
    std::cout << (ok ? "PASS  " : "FAIL  ") << name << "\n";
    if (!ok) {
        ++failures;
    }
}

bool near(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}

rb::Pid make(const rb::PidConfig& c)
{
    return rb::Pid::create(c).value();
}

} // namespace

int main()
{
    { // 1. invalid configurations give no controller
        rb::PidConfig zeroPeriod;
        zeroPeriod.period = 0.0;
        rb::PidConfig swapped;
        swapped.outMin = 1.0;
        swapped.outMax = -1.0;
        rb::PidConfig negative;
        negative.kp = -1.0;
        rb::PidConfig notANumber;
        notANumber.kd = std::numeric_limits<double>::quiet_NaN();
        check(!rb::Pid::create(zeroPeriod) && !rb::Pid::create(swapped) &&
                  !rb::Pid::create(negative) && !rb::Pid::create(notANumber),
              "invalid configurations are refused");
    }
    { // 2. P only
        rb::PidConfig c;
        c.kp = 0.5;
        rb::Pid pid = make(c);
        check(near(pid.update(1.0, 0.2), 0.4), "P only: u = kp * e");
    }
    { // 3. I accumulates ki * period * e per step
        rb::PidConfig c;
        c.ki = 2.0;
        c.period = 0.01;
        rb::Pid pid = make(c);
        double u = 0.0;
        for (int k = 0; k < 10; ++k) {
            u = pid.update(1.0, 0.0);
        }
        check(near(u, 0.2), "I only: ten steps of e = 1 give 2 * 0.01 * 10 = 0.2");
    }
    { // 4. D acts on the measurement: a setpoint step gives no kick
        rb::PidConfig c;
        c.kd = 1.0;
        c.outMin = -100.0;
        c.outMax = 100.0;
        rb::Pid pid = make(c);
        pid.update(0.0, 0.0);
        const double afterStep = pid.update(5.0, 0.0);
        const double ramp = pid.update(5.0, 0.01); // measurement rose 0.01 in 0.01 s
        check(near(afterStep, 0.0) && near(ramp, -1.0),
              "D: no kick on a setpoint step; -kd * slope on a ramp");
    }
    { // 5. derivative filter: alpha = 0.01 / (0.09 + 0.01) = 0.1 per step
        rb::PidConfig c;
        c.kd = 1.0;
        c.derivativeTau = 0.09;
        c.outMin = -100.0;
        c.outMax = 100.0;
        rb::Pid pid = make(c);
        pid.update(0.0, 0.0);
        double u = 0.0;
        for (int k = 1; k <= 3; ++k) {
            u = pid.update(0.0, 0.01 * k); // steady slope of 1 per second
        }
        check(near(u, -(1.0 - 0.9 * 0.9 * 0.9)),
              "filtered D after 3 steps = -(1 - 0.9^3) of the raw value");
    }
    { // 6. the output always stays inside the limits
        rb::PidConfig c;
        c.kp = 100.0;
        c.outMin = -12.0;
        c.outMax = 12.0;
        rb::Pid pid = make(c);
        const double high = pid.update(10.0, 0.0);
        const double low = pid.update(-10.0, 0.0);
        check(near(high, 12.0) && near(low, -12.0), "output clamped to [outMin, outMax]");
    }
    { // 7. anti-windup: 200 saturated steps, then the error reverses
        rb::PidConfig c;
        c.kp = 1.0;
        c.ki = 5.0;
        c.outMin = -1.0;
        c.outMax = 1.0;
        rb::Pid pid = make(c);
        for (int k = 0; k < 200; ++k) {
            pid.update(10.0, 0.0); // asks for far more than 1
        }
        const double integralWhileStuck = pid.integralTerm();
        const double afterReverse = pid.update(0.0, 0.5); // now e = -0.5
        check(integralWhileStuck <= 1.0 + 1e-12 && afterReverse < 1.0,
              std::format(
                  "anti-windup: integral held at {:.3f}, output leaves the limit at once ({:.3f})",
                  integralWhileStuck, afterReverse));
    }
    { // 8. a bad measurement is refused and does not poison the state
        rb::PidConfig c;
        c.kp = 1.0;
        c.ki = 1.0;
        c.kd = 0.1;
        c.outMin = -10.0;
        c.outMax = 10.0;
        rb::Pid pid = make(c);
        pid.update(1.0, 0.0);
        const double before = pid.lastOutput();
        const double during = pid.update(1.0, std::numeric_limits<double>::quiet_NaN());
        const double after = pid.update(1.0, 0.0);
        check(near(during, before) && std::isfinite(after) && pid.rejectedInputs() == 1,
              "NaN measurement refused, counted, and later outputs are finite");
    }
    { // 9. reset forgets the past
        rb::PidConfig c;
        c.ki = 10.0;
        rb::Pid pid = make(c);
        for (int k = 0; k < 5; ++k) {
            pid.update(1.0, 0.0);
        }
        pid.reset();
        check(near(pid.integralTerm(), 0.0) && near(pid.update(0.0, 0.0), 0.0),
              "reset clears integral and output");
    }
    { // 10. changing ki while running does not make the output jump
        rb::PidConfig c;
        c.ki = 1.0;
        c.outMin = -10.0;
        c.outMax = 10.0;
        rb::Pid pid = make(c);
        for (int k = 0; k < 50; ++k) {
            pid.update(1.0, 1.0 - 1e-12); // tiny error: integral almost constant
        }
        pid.update(1.0, 0.0);
        const double before = pid.lastOutput();
        pid.setGains(0.0, 4.0, 0.0);
        const double after = pid.update(1.0, 1.0); // zero error now
        check(near(before, after), "bumpless: new ki keeps the output where it was");
    }
    std::cout << std::format("{} test(s) failed\n", failures);
    return failures == 0 ? 0 : 1;
}
