// BR-08 Listing 5: close the gap one effect at a time.
//   ./resim identified.txt kit_run.csv
// Builds an updated simulator from the parameters that compare_logs.py identified in the
// kit's log (never from the stand-in's own source), runs the unchanged bench sequence on
// it, and compares each run with the kit's log. Each identified effect is added once alone
// (on top of the identified motor) and once cumulatively, in a fixed order.
#include "bench.hpp"
#include "kit_model.hpp"
#include <cmath>
#include <deque>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr double kPi = 3.141592653589793;

struct Effects
{
    bool motor = false, supply = false, delay = false, encoder = false, jitter = false,
         noise = false, ripple = false, frictionTorque = false;
};

// A first-order motor with a deadband: the F9-22 model's shape, the kit's identified numbers.
class SimV2 : public br08::LoggedMotorIo
{
public:
    SimV2(const std::map<std::string, double>& id, Effects e) : id_(id), e_(e), rng_(99) {}

    std::uint16_t readEncoder() override
    {
        stamp_ = clock_;
        const double cpr = e_.encoder ? id_.at("cpr_true_est") : 1024.0;
        double counts = angle_ / (2 * kPi) * cpr;
        if (e_.noise) {
            // rest noise (rad/s) -> counts at one period, split over two reads
            counts += id_.at("rest_noise") / (2 * kPi / 1024.0 / 0.01) / std::sqrt(2.0) *
                      rng_.gauss();
        }
        counts = std::floor(counts);
        return static_cast<std::uint16_t>(counts - 65536.0 * std::floor(counts / 65536.0));
    }
    void setDuty(double d) override
    {
        q_.push_back({clock_ + (e_.delay ? id_.at("dead_time") : 0.0), std::clamp(d, -1.0, 1.0)});
    }
    void advance(double s) override
    {
        nominal_ += s;
        const double late = e_.jitter ? id_.at("lateness_max_ms") / 1000.0 * rng_.uniform() : 0.0;
        while (clock_ < nominal_ + late - 1e-9) {
            while (!q_.empty() && q_.front().first <= clock_ + 1e-12) {
                duty_ = q_.front().second;
                q_.pop_front();
            }
            const double gain = e_.motor ? id_.at("gain_per_volt") : 98.04;
            const double tau = e_.motor ? id_.at("tau") : 0.1961;
            const double dead = e_.motor ? id_.at("deadband_v") : 0.5;
            double v = duty_ * volts() - (e_.ripple ? id_.at("ripple_v") * std::sin(angle_) : 0.0);
            if (!e_.frictionTorque) { // F9-22's structure: friction as a deadband on the input
                const double mag = std::max(0.0, std::abs(v) - dead);
                omega_ += 1e-4 * (gain * (v >= 0 ? mag : -mag) - omega_) / tau;
            } else { // friction as a torque that always opposes motion (and holds at rest)
                const double drive = gain * v / tau, fric = gain * dead / tau;
                if (std::abs(omega_) < 1e-3 && std::abs(drive) <= fric) {
                    omega_ = 0.0;
                } else {
                    const double dir = std::abs(omega_) < 1e-3 ? (drive > 0 ? 1.0 : -1.0)
                                                               : (omega_ > 0 ? 1.0 : -1.0);
                    omega_ += 1e-4 * (drive - omega_ / tau - fric * dir);
                }
            }
            angle_ += 1e-4 * omega_;
            clock_ += 1e-4;
        }
    }
    double sampleTime() const override { return stamp_; }
    double supplyVolts() const override { return volts(); }
    double referenceSpeed() const override { return omega_; }
    std::string world() const override { return "updated simulator (resim.cc)"; }

private:
    double volts() const { return e_.supply ? id_.at("supply_rest_v") : 12.0; }
    const std::map<std::string, double>& id_;
    Effects e_;
    br08::Rng rng_;
    std::deque<std::pair<double, double>> q_;
    double duty_ = 0, omega_ = 0, angle_ = 1000.0, clock_ = 0, nominal_ = 0, stamp_ = 0;
};

struct Trace
{
    std::vector<double> meas, ref;
};

Trace loopOf(const br08::Result& r)
{
    Trace t;
    for (const br08::Row& w : r.rows) {
        if (w.phase == "loop") {
            t.meas.push_back(w.measured);
            t.ref.push_back(w.reference);
        }
    }
    return t;
}

Trace loopOfFile(const std::string& path)
{
    Trace t;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("loop,", 0) != 0) {
            continue;
        }
        std::stringstream ss(line);
        std::string f[8];
        for (auto& x : f) {
            std::getline(ss, x, ',');
        }
        t.meas.push_back(std::stod(f[4]));
        t.ref.push_back(std::stod(f[7]));
    }
    return t;
}

double movingAverage(const std::vector<double>& x, std::size_t i)
{
    double s = 0;
    int n = 0;
    for (std::size_t j = (i >= 2 ? i - 2 : 0); j <= i + 2 && j < x.size(); ++j, ++n) {
        s += x[j];
    }
    return s / n;
}

struct Score
{
    double overshoot = 0, refHold = 0, errStd = 0, rms = 0;
    double seg[3] = {0, 0, 0}; // rms in 0-1.2 s, 1.2-2.4 s, 2.4-3.0 s of the loop
};

Score score(const Trace& t, const Trace& kit)
{
    Score s;
    double peak = 0, sum = 0, e = 0, e2 = 0, d2 = 0;
    for (std::size_t i = 0; i < 120 && i < t.meas.size(); ++i) {
        peak = std::max(peak, t.meas[i]);
    }
    for (std::size_t i = 80; i < 120; ++i) {
        sum += t.ref[i];
    }
    for (std::size_t i = 200; i < 240; ++i) {
        e += t.meas[i] - 250.0;
        e2 += (t.meas[i] - 250.0) * (t.meas[i] - 250.0);
    }
    const std::size_t n = std::min(t.ref.size(), kit.ref.size());
    double segSum[3] = {0, 0, 0};
    int segN[3] = {0, 0, 0};
    for (std::size_t i = 0; i < n; ++i) {
        const double d = movingAverage(t.ref, i) - movingAverage(kit.ref, i);
        d2 += d * d;
        const int g = i < 120 ? 0 : (i < 240 ? 1 : 2);
        segSum[g] += d * d;
        ++segN[g];
    }
    for (int g = 0; g < 3; ++g) {
        s.seg[g] = segN[g] ? std::sqrt(segSum[g] / segN[g]) : 0.0;
    }
    s.overshoot = 100.0 * (peak - 150.0) / 150.0;
    s.refHold = sum / 40.0;
    s.errStd = std::sqrt(std::max(0.0, e2 / 40.0 - (e / 40.0) * (e / 40.0)));
    s.rms = std::sqrt(d2 / static_cast<double>(n));
    return s;
}

Score runWith(const std::map<std::string, double>& id, Effects e, const Trace& kit)
{
    br08::BenchPlan plan;
    plan.signDuty = 0.2; // the plan the kit log was recorded with
    br08::Result r;
    if (!e.motor) {
        br08::SimWorld sim; // the original simulator, unchanged
        r = br08::Bench(sim, plan).run();
    } else {
        SimV2 sim(id, e);
        r = br08::Bench(sim, plan).run();
    }
    return score(loopOf(r), kit);
}

void print(const std::string& label, const Score& s)
{
    std::cout << std::format("{:<32} {:>6.1f} {:>8.2f} {:>6.2f} {:>6.2f} = "
                             "{:>5.2f} {:>5.2f} {:>5.2f}\n",
                             label, s.overshoot, s.refHold, s.errStd, s.rms, s.seg[0], s.seg[1],
                             s.seg[2]);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "usage: resim identified.txt kit_run.csv\n";
        return 2;
    }
    std::map<std::string, double> id;
    std::ifstream in(argv[1]);
    std::string key;
    double value = 0;
    while (in >> key >> value) {
        id[key] = value;
    }
    const Trace kit = loopOfFile(argv[2]);
    if (kit.ref.size() < 240 || id.size() < 10) {
        std::cerr << "input incomplete\n";
        return 1;
    }
    const Score k = score(kit, kit);
    std::cout << "columns: overshoot at 150 (%), reference in the 150 hold (rad/s),\n"
                 "         measured error std in the 250 hold (rad/s), rms distance of the\n"
                 "         reference trace from the kit's (5-sample average, rad/s)\n\n";
    std::cout << std::format("{:<32} {:>6} {:>8} {:>6} {:>6}   {:>5} {:>5} {:>5}\n", "run", "over%",
                             "ref150", "err", "rms", "150", "250", "down");
    print("kit log (the target)", k);
    print("0 original simulator (F9-22)", runWith(id, {}, kit));

    const std::vector<std::pair<std::string, bool Effects::*>> steps = {
        {"supply 11.16 V", &Effects::supply}, {"dead time", &Effects::delay},
        {"encoder 1000 counts", &Effects::encoder}, {"timing jitter", &Effects::jitter},
        {"encoder noise", &Effects::noise},       {"ripple", &Effects::ripple}};
    Effects base;
    base.motor = true;
    std::cout << "\neach effect alone, on top of the identified motor:\n";
    print("1 identified motor only", runWith(id, base, kit));
    for (const auto& [name, field] : steps) {
        Effects e = base;
        e.*field = true;
        print("  + " + name + " (alone)", runWith(id, e, kit));
    }
    std::cout << "\ncumulative, one effect added per row:\n";
    Effects cum = base;
    print("1 identified motor", runWith(id, cum, kit));
    int n = 2;
    for (const auto& [name, field] : steps) {
        cum.*field = true;
        print(std::format("{} + {}", n++, name), runWith(id, cum, kit));
    }
    std::cout << "\nstructure change (not a parameter): friction as an opposing torque\n";
    cum.frictionTorque = true;
    print(std::format("{} + friction as torque", n), runWith(id, cum, kit));
    return 0;
}
