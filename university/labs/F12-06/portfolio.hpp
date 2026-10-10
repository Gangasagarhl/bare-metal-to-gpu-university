// portfolio.hpp - the test levels of F12-06 and the variants they are run against.
// A variant is the system with zero or one planted bug (a "mutant"). Every test level is a
// function that runs its checks against a variant and returns how many checks failed and how
// much work it did (function calls or simulation steps), so the cost of each level is counted
// the same way on every machine.
#pragma once
#include "speedctl.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

namespace portfolio {

struct Variant {
    const char* name;
    const char* bug;
    std::optional<double> (*parse)(std::string_view);
    double (*gov)(double);
    double parser_unit_scale;  // what the parser's own unit tests expect per metre (1 = metres)
    double sensor_delay_s;     // age of the reading the control loop uses
};

// ---- planted bugs ---------------------------------------------------------------------------
inline std::optional<double> parse_mm(std::string_view p)  // M2: returns millimetres
{
    auto m = speedctl::parse_range_m(p);
    if (!m) {
        return std::nullopt;
    }
    return *m * 1000.0;
}

inline std::optional<double> parse_lenient(std::string_view p)  // M4: bad digits read as 0
{
    auto m = speedctl::parse_range_m(p);
    if (!m && p.size() >= 2 && p[0] == 'R') {
        return 0.0;
    }
    return m;
}

inline double gov_noclamp(double d)  // M1: the upper clamp was deleted
{
    if (d <= speedctl::kStop) {
        return 0.0;
    }
    return speedctl::kVmax * (d - speedctl::kStop) / (speedctl::kFull - speedctl::kStop);
}

inline double gov_typo(double d)  // M5: stop distance typed as 0.05 instead of 0.5
{
    if (d <= 0.05) {
        return 0.0;
    }
    if (d >= speedctl::kFull) {
        return speedctl::kVmax;
    }
    return speedctl::kVmax * (d - 0.05) / (speedctl::kFull - 0.05);
}

inline const Variant kVariants[] = {
    {"OK", "no planted bug", speedctl::parse_range_m, speedctl::governor, 1.0, 0.0},
    {"M1", "governor: upper clamp deleted", speedctl::parse_range_m, gov_noclamp, 1.0, 0.0},
    {"M2", "parser returns mm (its own tests say mm)", parse_mm, speedctl::governor, 1000.0, 0.0},
    {"M3", "control loop uses a 0.6 s old reading", speedctl::parse_range_m, speedctl::governor,
     1.0, 0.6},
    {"M4", "parser reads malformed digits as 0 mm", parse_lenient, speedctl::governor, 1.0, 0.0},
    {"M5", "governor: stop distance 0.05 m", speedctl::parse_range_m, gov_typo, 1.0, 0.0},
};

struct Result {
    int failed = 0;
    long work = 0;  // function calls (unit, integration) or simulation steps (system)
};

inline bool near(double a, double b)
{
    return std::fabs(a - b) < 1e-9;
}

// ---- level 1: unit tests, each unit alone against its own specification ----------------------
inline Result unit_parser(const Variant& v)
{
    Result r;
    const double s = v.parser_unit_scale;
    auto check = [&](bool ok) { r.failed += ok ? 0 : 1; ++r.work; };
    auto is = [&](const char* p, double want) { auto m = v.parse(p); check(m && near(*m, want)); };
    auto bad = [&](const char* p) { check(!v.parse(p)); };
    is("R 1500", 1.5 * s);
    is("R 0", 0.0);
    is("R 65535", 65.535 * s);
    bad("R 65536");
    bad("R ");
    bad("X 100");
    bad("R 12a");
    return r;
}

inline Result unit_governor(const Variant& v)
{
    Result r;
    auto is = [&](double d, double want) { r.failed += near(v.gov(d), want) ? 0 : 1; ++r.work; };
    is(0.0, 0.0);
    is(0.4, 0.0);
    is(0.5, 0.0);
    is(0.75, 0.5);
    is(1.0, 1.0);
    is(3.0, 1.0);
    return r;
}

// ---- level 2: integration, parser and governor together through the controller --------------
// The controller's rule: a packet it cannot parse means "stop" (fail-safe).
inline double command_for(const Variant& v, std::string_view packet)
{
    auto d = v.parse(packet);
    return d ? v.gov(*d) : 0.0;
}

inline Result integration(const Variant& v)
{
    Result r;
    auto is = [&](const char* p, double want) {
        r.failed += near(command_for(v, p), want) ? 0 : 1;
        r.work += 2;
    };
    is("R 4000", 1.0);
    is("R 750", 0.5);
    is("R 300", 0.0);
    is("garbage", 0.0);
    return r;
}

// ---- level 3: system test, the closed loop in a simulated world ------------------------------
struct Trace {
    double min_d = 1e9, max_v = 0.0, final_d = 0.0, final_v = 0.0;
    bool contact = false;
};

constexpr double kDt = 0.01;        // physics step, s
constexpr int kControlEvery = 5;    // control period = 5 steps = 0.05 s
constexpr double kAccel = 2.0;      // motor speed change limit, m/s^2
constexpr double kStart = 4.0;      // start distance, m
constexpr double kDuration = 20.0;  // s

inline Trace simulate(const Variant& v, double start_d, long& steps)
{
    Trace t;
    double d = start_d, vel = 0.0, cmd = 0.0;
    const int delay_steps = static_cast<int>(std::lround(v.sensor_delay_s / kDt));
    std::deque<double> history;  // true distance at each step, newest at the back
    const int n = static_cast<int>(std::lround(kDuration / kDt));
    for (int i = 0; i < n; ++i) {
        history.push_back(d);
        if (static_cast<int>(history.size()) > delay_steps + 1) {
            history.pop_front();
        }
        if (i % kControlEvery == 0) {
            const double seen = history.front();  // the reading is delay_steps old
            const long mm = std::lround(seen * 1000.0);
            const std::string packet = "R " + std::to_string(mm < 0 ? 0 : mm);
            cmd = command_for(v, packet);
        }
        const double dv = std::clamp(cmd - vel, -kAccel * kDt, kAccel * kDt);
        vel += dv;
        d -= vel * kDt;
        ++steps;
        t.min_d = std::min(t.min_d, d);
        t.max_v = std::max(t.max_v, vel);
        if (d <= 0.0) {
            t.contact = true;
            break;
        }
    }
    t.final_d = d;
    t.final_v = vel;
    return t;
}

// System requirements: S1 never closer than 0.35 m; S2 at the end stopped (< 0.02 m/s) within
// 0.65 m of the wall; S3 never faster than kVmax.
inline Result system_test(const Variant& v, double start_d = kStart)
{
    Result r;
    long steps = 0;
    const Trace t = simulate(v, start_d, steps);
    r.work = steps;
    r.failed += (t.contact || t.min_d < 0.35) ? 1 : 0;
    r.failed += (t.final_v < 0.02 && t.final_d <= 0.65) ? 0 : 1;
    r.failed += (t.max_v <= speedctl::kVmax + 1e-9) ? 0 : 1;
    return r;
}

}  // namespace portfolio
