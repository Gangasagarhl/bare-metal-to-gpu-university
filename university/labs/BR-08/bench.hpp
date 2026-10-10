// BR-08 Listing 1: what carries over from the simulator to the kit.
// The control code of F9-22 is reused unchanged: the MotorIo boundary, speedFromCounts,
// the F9-21 PID library and the software safety layers (sign test, speed limit, ramp,
// stall watchdog). The bridge adds only two things, so that every world produces the
// same evidence:
//   1. LoggedMotorIo: three measurements every world must provide: the time stamp of
//      each sample, the supply voltage, and a reference speed measured outside the
//      control path (on the bench a tachometer; in a simulator the true speed);
//   2. one bench sequence and one log format, used identically in every world.
#pragma once
#include "../F9-21/pid.hpp"
#include "../F9-22/motor_io.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace br08 {

class LoggedMotorIo : public MotorIo
{
public:
    virtual double sampleTime() const = 0;     // s, clock at the last readEncoder()
    virtual double supplyVolts() const = 0;    // V, supply measured at that moment
    virtual double referenceSpeed() const = 0; // rad/s, measured outside the control path
    virtual std::string world() const = 0;     // written into the run record
};

// The F9-22 simulated kit, unchanged, behind the extended interface.
class SimWorld : public LoggedMotorIo
{
public:
    std::uint16_t readEncoder() override
    {
        stamp_ = clock_;
        return kit_.readEncoder();
    }
    void setDuty(double duty) override { kit_.setDuty(duty); }
    void advance(double seconds) override
    {
        kit_.advance(seconds);
        clock_ += seconds;
    }
    double sampleTime() const override { return stamp_; }
    double supplyVolts() const override { return kit_.supplyVolts; }
    double referenceSpeed() const override { return kit_.trueSpeed(); }
    std::string world() const override { return "simulator (F9-22 SimulatedKit, unchanged)"; }

private:
    SimulatedKit kit_;
    double clock_ = 0.0;
    double stamp_ = 0.0;
};

struct Gains
{
    double kp = 0.0; // V per rad/s
    double ki = 0.0; // V per rad
};
inline constexpr Gains kSimGains{0.0482, 0.2474}; // identified on the simulator in F9-22

struct BenchPlan
{
    double period = 0.01;        // s
    double nominalVolts = 12.0;  // duty = volts / nominalVolts, as in F9-22
    double cprParam = 1024.0;    // counts per revolution the software believes
    double speedLimit = 400.0;   // rad/s, cap on the setpoint (F9-22)
    double rampPerSecond = 600.0; // rad/s per s (F9-22)
    double overspeed = 450.0;    // rad/s, stop if |measured| exceeds it
    int averageSamples = 1;      // moving average of the measured speed (1 = none)
    bool timestampSpeed = false; // divide by the measured interval, not by the period
    Gains gains = kSimGains;
    bool benchSteps = true;      // sign test and open-loop step test before the loop
    double signDuty = 0.1;       // duty of the sign test (F9-22)
    double loopSeconds = 3.0;
    std::vector<std::pair<double, double>> profile = {{1.2, 150.0}, {2.4, 250.0}, {3.0, 0.0}};
};

struct Row
{
    std::string phase;
    double tNominal = 0, tSample = 0, setpoint = 0, measured = 0, duty = 0, volts = 0,
           reference = 0;
};

struct Result
{
    std::vector<Row> rows;
    std::string stop = "completed"; // or the reason the sequence stopped
};

class Bench
{
public:
    Bench(LoggedMotorIo& io, const BenchPlan& plan) : io_(io), plan_(plan) {}

    Result run()
    {
        before_ = io_.readEncoder();
        if (plan_.benchSteps) {
            openLoop("rest", 0.0, 0.3);
            const std::uint16_t start = before_;
            openLoop("sign", plan_.signDuty, 0.2);
            const double avg = speedFromCounts(before_, start, plan_.cprParam, 0.2);
            if (avg <= 5.0) {
                return stop(std::format("sign test failed ({:+.1f} rad/s)", avg));
            }
            openLoop("coast", 0.0, 1.0);
            openLoop("step", 0.25, 1.0);
            openLoop("coast", 0.0, 0.6);
        }
        closedLoop();
        io_.setDuty(0.0); // every exit leaves the motor commanded off
        return result_;
    }

private:
    double measure()
    {
        const std::uint16_t now = io_.readEncoder();
        const double interval = io_.sampleTime() - lastStamp_;
        lastStamp_ = io_.sampleTime();
        const bool useStamp = plan_.timestampSpeed && interval > 0.5 * plan_.period;
        const double speed =
            speedFromCounts(now, before_, plan_.cprParam, useStamp ? interval : plan_.period);
        before_ = now;
        recent_.push_back(speed);
        if (static_cast<int>(recent_.size()) > std::max(1, plan_.averageSamples)) {
            recent_.erase(recent_.begin());
        }
        double sum = 0.0;
        for (double v : recent_) {
            sum += v;
        }
        return sum / static_cast<double>(recent_.size());
    }

    void log(const char* phase, double setpoint, double measured, double duty)
    {
        result_.rows.push_back({phase, t_, io_.sampleTime(), setpoint, measured, duty,
                                io_.supplyVolts(), io_.referenceSpeed()});
    }

    void openLoop(const char* phase, double duty, double seconds)
    {
        const int n = static_cast<int>(std::lround(seconds / plan_.period));
        for (int k = 0; k < n; ++k) {
            const double measured = measure();
            io_.setDuty(duty);
            log(phase, 0.0, measured, duty);
            tick();
        }
    }

    void closedLoop()
    {
        rb::PidConfig c;
        c.kp = plan_.gains.kp;
        c.ki = plan_.gains.ki;
        c.period = plan_.period;
        c.outMin = -plan_.nominalVolts;
        c.outMax = plan_.nominalVolts;
        rb::Pid pid = rb::Pid::create(c).value();
        double setpoint = 0.0;
        double stalledFor = 0.0;
        const int n = static_cast<int>(std::lround(plan_.loopSeconds / plan_.period));
        for (int k = 0; k < n; ++k) {
            const double tLoop = k * plan_.period;
            double goal = 0.0;
            for (const auto& [until, value] : plan_.profile) {
                if (tLoop < until) {
                    goal = std::min(value, plan_.speedLimit);
                    break;
                }
            }
            const double step = plan_.rampPerSecond * plan_.period;
            setpoint = std::clamp(goal, setpoint - step, setpoint + step);
            const double measured = measure();
            if (std::abs(measured) > plan_.overspeed) {
                log("loop", setpoint, measured, 0.0);
                stop(std::format("overspeed: measured {:.1f} rad/s", measured));
                return;
            }
            const double duty = pid.update(setpoint, measured) / plan_.nominalVolts;
            const bool stalled = std::abs(duty) > 0.5 && std::abs(measured) < 5.0;
            stalledFor = stalled ? stalledFor + plan_.period : 0.0;
            if (stalledFor >= 0.3) {
                log("loop", setpoint, measured, 0.0);
                stop("stall watchdog");
                return;
            }
            io_.setDuty(duty);
            log("loop", setpoint, measured, duty);
            tick();
        }
    }

    Result stop(const std::string& why)
    {
        io_.setDuty(0.0);
        result_.stop = why;
        return result_;
    }

    void tick()
    {
        io_.advance(plan_.period);
        t_ += plan_.period;
    }

    LoggedMotorIo& io_;
    BenchPlan plan_;
    Result result_;
    std::uint16_t before_ = 0;
    std::vector<double> recent_; // the last averageSamples speeds
    double lastStamp_ = 0.0;
    double t_ = 0.0;
};

// One log format for every world: a run record (conditions) and one row per period.
inline void writeLog(std::ostream& out, const LoggedMotorIo& io, const BenchPlan& plan,
                     const Result& r, const std::string& conditions)
{
    out << "# BR-08 run record\n";
    out << "# world: " << io.world() << '\n';
    out << "# conditions: " << conditions << '\n';
    out << "# controller: bench.hpp (F9-21 PID, F9-22 safety layers), duty = volts / "
        << plan.nominalVolts << '\n';
    out << std::format("# gains: kp={} ki={}\n", plan.gains.kp, plan.gains.ki);
    out << std::format("# period_s: {}  cpr_param: {}  average_samples: {}  timestamp_speed: {}\n",
                       plan.period, plan.cprParam, plan.averageSamples, plan.timestampSpeed);
    std::string profile;
    for (const auto& [until, value] : plan.profile) {
        profile += std::format("{}{} rad/s until {} s", profile.empty() ? "" : ", ", value, until);
    }
    out << std::format("# scenario: rest 0.3 s, sign 0.2 s at duty {}, coast 1.0 s, step 1.0 s at"
                       " duty 0.25, coast 0.6 s, loop {} s ({}; ramp {} rad/s^2)\n",
                       plan.signDuty, plan.loopSeconds, profile, plan.rampPerSecond);
    out << "# stop: " << r.stop << '\n';
    out << "phase,t_nom,t_sample,setpoint,measured,duty,volts,reference\n";
    for (const Row& w : r.rows) {
        out << std::format("{},{:.3f},{:.5f},{:.2f},{:.2f},{:.4f},{:.2f},{:.2f}\n", w.phase,
                           w.tNominal, w.tSample, w.setpoint, w.measured, w.duty, w.volts,
                           w.reference);
    }
}

} // namespace br08
