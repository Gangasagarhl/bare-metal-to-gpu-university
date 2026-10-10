// dronesim.hpp - the DN401 course simulator.
// A point-mass multirotor with a battery, a compass, GNSS, an RC link, and a small
// flight software (heading estimator, position controller, modes, failsafes).
// Teaching model only: it is NOT PX4 and NOT ArduPilot. Every number in this file is an
// invented teaching value, not a property of any real vehicle, battery or sensor.
// Frames: x north, y east, z up. Heading (yaw) is measured from north towards east.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace dn {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDt = 0.01;  // one simulation and control step, s (100 Hz, lockstep)
constexpr double kG = 9.81;   // m/s^2

struct V3
{
    double x = 0, y = 0, z = 0;
};
inline V3 operator+(V3 a, V3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
inline V3 operator-(V3 a, V3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline V3 operator*(double k, V3 a)
{
    return {k * a.x, k * a.y, k * a.z};
}
inline double hnorm(V3 a)
{
    return std::hypot(a.x, a.y);
}

// Rotate the horizontal part of a by angle ang (rad), north towards east.
inline V3 rotz(V3 a, double ang)
{
    const double c = std::cos(ang), s = std::sin(ang);
    return {c * a.x - s * a.y, s * a.x + c * a.y, a.z};
}

inline double wrap(double a)  // to (-pi, pi]
{
    while (a > kPi) a -= 2 * kPi;
    while (a <= -kPi) a += 2 * kPi;
    return a;
}

inline V3 limit_h(V3 a, double max)  // limit the horizontal length, keep z
{
    const double n = hnorm(a);
    if (n > max) {
        a.x *= max / n;
        a.y *= max / n;
    }
    return a;
}

// Deterministic noise (xorshift64* and Box-Muller): same seed, same flight, any machine.
struct Rng
{
    std::uint64_t s;
    explicit Rng(std::uint64_t seed) : s(seed ? seed : 1) {}
    double uniform()
    {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        return static_cast<double>((s * 2685821657736338717ULL) >> 11) * (1.0 / 9007199254740992.0);
    }
    double gauss()
    {
        const double u1 = std::max(uniform(), 1e-12), u2 = uniform();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
    }
};

// Battery: invented open-circuit curve per cell plus a series resistance (sag under load).
struct Battery
{
    int cells = 4;
    double capacity_mah = 5000;  // what the simulated pack really holds
    double r_int = 0.025;        // pack resistance, ohm
    double used_mah = 0;
    double soc() const { return std::clamp(1.0 - used_mah / capacity_mah, 0.0, 1.0); }
    static double ocv_cell(double soc)
    {
        static const double S[] = {0.0, 0.1, 0.2, 0.5, 0.8, 1.0};
        static const double V[] = {3.30, 3.55, 3.68, 3.80, 3.95, 4.15};
        for (int i = 1; i < 6; ++i)
            if (soc <= S[i])
                return V[i - 1] + (V[i] - V[i - 1]) * (soc - S[i - 1]) / (S[i] - S[i - 1]);
        return V[5];
    }
    double volts(double amps) const { return cells * ocv_cell(soc()) - amps * r_int; }
    void drain(double amps, double dt) { used_mah += amps * dt * 1000.0 / 3600.0; }
};

struct Sensors
{
    double t = 0;
    V3 gps_pos, gps_vel;
    double mag_heading = 0, mag_norm = 1;  // field normalised: undisturbed strength = 1.0
    double volts = 0, amps = 0;
    bool rc_valid = true;  // what the receiver output says
    int rc_lq = 100;       // link quality the receiver reports (0..100)
};

struct Command
{
    V3 acc;  // requested acceleration: x,y in the heading frame the software believes
    bool armed = false;
};

// The world: vehicle truth, battery, sensors, radio. In HITL this part stays on the PC.
struct World
{
    V3 pos, vel, acc;    // truth; acc = acceleration produced by the thrust vector
    double yaw = 0;      // true heading, rad (no yaw control in this model)
    V3 wind;             // wind velocity, m/s
    double drag = 0.35;  // 1/s
    double tau = 0.15;   // thrust-vector response time constant, s
    Battery batt;
    double amps = 0;
    double mag_int_per_amp = 0;    // body-frame interference field per amp (normalised units)
    double mag_int_dir = 0;        // its direction in the body frame, rad
    bool rc_link = true;           // false: the transmitter's signal is gone
    bool rc_hold_on_loss = false;  // receiver set to repeat its last output instead of stopping
    bool depleted = false, crashed = false, on_ground = true;
    double t = 0;
    Rng rng{42};

    Sensors sense()
    {
        Sensors s;
        s.t = t;
        s.gps_pos = pos + V3{0.15 * rng.gauss(), 0.15 * rng.gauss(), 0.10 * rng.gauss()};
        s.gps_vel = vel + V3{0.05 * rng.gauss(), 0.05 * rng.gauss(), 0.05 * rng.gauss()};
        const double d = mag_int_per_amp * amps;  // interference grows with motor current
        const double bx = std::cos(yaw) + d * std::cos(mag_int_dir);
        const double by = -std::sin(yaw) + d * std::sin(mag_int_dir);
        s.mag_heading = wrap(std::atan2(-by, bx) + 0.01 * rng.gauss());
        s.mag_norm = std::hypot(bx, by);
        s.amps = amps + 0.2 * rng.gauss();
        s.volts = batt.volts(amps) + 0.02 * rng.gauss();
        s.rc_valid = rc_link || rc_hold_on_loss;
        s.rc_lq = rc_link ? 100 : 0;
        return s;
    }

    void step(const Command& c)
    {
        t += kDt;
        V3 req = rotz(c.acc, yaw);  // the vehicle tilts in its TRUE frame
        if (!c.armed) req = V3{0, 0, -kG};
        if (depleted) {
            req = limit_h(req, 1.0);
            req.z = std::min(req.z, -3.0);
        }
        acc = acc + (kDt / tau) * (req - acc);
        if (on_ground && acc.z <= 0) {
            vel = V3{};
            acc = V3{};
        } else {
            on_ground = false;
            vel = vel + kDt * (acc + drag * (wind - vel));
            pos = pos + kDt * vel;
            if (pos.z <= 0) {
                if (vel.z < -2.5) crashed = true;
                pos.z = 0;
                vel = V3{};
                acc = V3{};
                on_ground = true;
            }
        }
        // Electrical power grows with thrust^1.5; current = power / loaded voltage, so the
        // current RISES as the pack's voltage falls (solve V = Voc - I*R for I).
        const double thrust = std::hypot(kG + acc.z, hnorm(acc)) / kG;  // 1.0 = hover thrust
        const double watts = c.armed ? 265.0 * std::pow(std::max(thrust, 0.0), 1.5) + 8.0 : 5.0;
        const double voc = batt.volts(0.0);
        const double disc = voc * voc - 4.0 * batt.r_int * watts;
        amps = disc > 0 ? (voc - std::sqrt(disc)) / (2.0 * batt.r_int) : voc / (2.0 * batt.r_int);
        batt.drain(amps, kDt);
        if (batt.soc() <= 0.02 || batt.volts(amps) < 3.0 * batt.cells) depleted = c.armed;
    }
};

// ---------------- flight software (in HITL this part runs on the flight controller) ------
enum class Mode { Disarmed, Takeoff, Mission, Hold, RTL, Land };
inline const char* name(Mode m)
{
    switch (m) {
    case Mode::Disarmed:
        return "DISARMED";
    case Mode::Takeoff:
        return "TAKEOFF";
    case Mode::Mission:
        return "MISSION";
    case Mode::Hold:
        return "HOLD";
    case Mode::RTL:
        return "RTL";
    case Mode::Land:
        return "LAND";
    }
    return "?";
}

// Settings. The names are this simulator's own, NOT PX4 or ArduPilot parameter names.
struct Params
{
    double kp = 0.9, kv = 1.8;  // position (1/s) and velocity (1/s) gains
    double vmax_h = 5, vmax_z = 2, amax_h = 4, amax_z = 3;
    double takeoff_alt = 10, rtl_alt = 15, land_speed = 0.7, accept_radius = 1.0;
    double yaw_gain = 0.5;       // compass correction gain of the heading estimator, 1/s
    double rc_timeout = 1.0;     // s without valid RC before the RC-loss failsafe
    Mode rc_action = Mode::RTL;  // Hold, RTL, Land, or Mission (= continue)
    int cells = 4;
    double batt_capacity_mah = 5000;  // what the software is TOLD the pack holds
    double batt_low_pct = 30, batt_crit_pct = 15;
    double batt_low_cell_v = 0;  // per-cell loaded-voltage trigger (0 = off)
    Mode batt_low_action = Mode::RTL, batt_crit_action = Mode::Land;
    double fence_radius = 0, fence_alt = 0;  // 0 = no fence
    Mode fence_action = Mode::RTL;
};

inline int rank(Mode m)  // failsafes may only escalate: Mission < Hold < RTL < Land
{
    switch (m) {
    case Mode::Hold:
        return 1;
    case Mode::RTL:
        return 2;
    case Mode::Land:
        return 3;
    default:
        return 0;
    }
}

struct Event
{
    double t;
    std::string text;
};

struct Fsw
{
    Params p;
    Mode mode = Mode::Disarmed;
    std::vector<V3> mission;
    std::size_t wp = 0;
    V3 home, pos, vel, sp, acc_cmd_earth;
    double yaw_est = 0, last_rc_t = 0, used_mah = 0, low_v_since = -1;
    int rtl_phase = 0, fs_rank = 0;
    bool yaw_init = false, low_done = false, crit_done = false;
    std::vector<Event> events;

    double remaining_pct() const { return 100.0 * (1.0 - used_mah / p.batt_capacity_mah); }

    void log(double t, const std::string& s) { events.push_back({t, s}); }

    void enter(Mode m, double t, const std::string& why)
    {
        if (m == mode) return;
        log(t, std::string(name(mode)) + " -> " + name(m) + " (" + why + ")");
        mode = m;
        if (m == Mode::Hold) sp = pos;
        if (m == Mode::RTL) {
            rtl_phase = 0;
            sp = {pos.x, pos.y, std::max(pos.z, p.rtl_alt)};
        }
        if (m == Mode::Land) sp = pos;
    }

    void failsafe(Mode action, double t, const std::string& why)
    {
        if (action == Mode::Mission) {
            log(t, "failsafe ignored by setting (" + why + ")");
            return;
        }
        if (rank(action) <= fs_rank) return;  // never de-escalate
        fs_rank = rank(action);
        enter(action, t, "failsafe: " + why);
    }

    // Pre-arm checks: refuse to arm unless the inputs look sane ON THE GROUND.
    std::vector<std::string> prearm(const Sensors& s) const
    {
        std::vector<std::string> f;
        if (!s.rc_valid) f.push_back("no valid RC input");
        if (s.mag_norm < 0.8 || s.mag_norm > 1.2)
            f.push_back("compass field strength inconsistent");
        if (s.volts / p.cells < 3.8) f.push_back("battery voltage low for take-off");
        return f;
    }

    void arm(const Sensors& s, std::vector<V3> m)
    {
        home = s.gps_pos;
        home.z = 0;
        pos = s.gps_pos;
        mission = std::move(m);
        wp = 0;
        last_rc_t = s.t;
        used_mah = 0;
        fs_rank = 0;
        sp = {home.x, home.y, p.takeoff_alt};
        enter(Mode::Takeoff, s.t, "armed");
    }

    Command step(const Sensors& s)
    {
        pos = s.gps_pos;
        vel = s.gps_vel;
        if (!yaw_init) {
            yaw_est = s.mag_heading;
            yaw_init = true;
        }
        yaw_est = wrap(yaw_est + p.yaw_gain * kDt * wrap(s.mag_heading - yaw_est));
        if (mode == Mode::Disarmed) return {};
        used_mah += s.amps * kDt * 1000.0 / 3600.0;

        // ---- failsafe monitors
        if (s.rc_valid) last_rc_t = s.t;
        if (s.t - last_rc_t > p.rc_timeout) failsafe(p.rc_action, s.t, "RC lost");
        const double rem = remaining_pct();
        if (!crit_done && rem < p.batt_crit_pct) {
            crit_done = true;
            failsafe(p.batt_crit_action, s.t, "battery critical");
        } else if (!low_done && rem < p.batt_low_pct) {
            low_done = true;
            failsafe(p.batt_low_action, s.t, "battery low");
        }
        if (p.batt_low_cell_v > 0) {
            if (s.volts / p.cells < p.batt_low_cell_v) {
                if (low_v_since < 0) low_v_since = s.t;
                if (s.t - low_v_since > 2.0 && !low_done) {
                    low_done = true;
                    failsafe(p.batt_low_action, s.t, "battery voltage low");
                }
            } else
                low_v_since = -1;
        }
        const V3 off = pos - home;
        if ((p.fence_radius > 0 && hnorm(off) > p.fence_radius) ||
            (p.fence_alt > 0 && pos.z > p.fence_alt))
            failsafe(p.fence_action, s.t, "geofence breach");

        // ---- modes produce a position setpoint (Land commands a descent speed instead)
        double vz_override = 0;
        bool use_vz = false;
        switch (mode) {
        case Mode::Takeoff:
            if (std::fabs(pos.z - p.takeoff_alt) < 0.5)
                enter(mission.empty() ? Mode::Hold : Mode::Mission, s.t,
                      "take-off altitude reached");
            break;
        case Mode::Mission:
            sp = mission[wp];
            if (hnorm(sp - pos) < p.accept_radius && std::fabs(sp.z - pos.z) < p.accept_radius) {
                log(s.t, "waypoint " + std::to_string(wp + 1) + " reached");
                if (++wp == mission.size()) enter(Mode::RTL, s.t, "mission complete");
            }
            break;
        case Mode::RTL:
            if (rtl_phase == 0 && pos.z > sp.z - 0.5) {
                rtl_phase = 1;
                sp = {home.x, home.y, sp.z};
            }
            if (rtl_phase == 1 && hnorm(sp - pos) < p.accept_radius)
                enter(Mode::Land, s.t, "home reached");
            break;
        case Mode::Land:
            use_vz = true;
            vz_override = -p.land_speed;
            if (pos.z < 0.3 && std::fabs(vel.z) < 0.3) {
                enter(Mode::Disarmed, s.t, "landed");
                return {};
            }
            break;
        default:
            break;
        }

        // ---- position controller -> velocity setpoint -> acceleration (earth frame)
        V3 vsp = limit_h(p.kp * (sp - pos), p.vmax_h);
        vsp.z = use_vz ? vz_override : std::clamp(p.kp * (sp.z - pos.z), -p.vmax_z, p.vmax_z);
        V3 a = limit_h(p.kv * (vsp - vel), p.amax_h);
        a.z = std::clamp(p.kv * (vsp.z - vel.z), -p.amax_z, p.amax_z);
        acc_cmd_earth = a;
        // Convert to the heading frame with the ESTIMATED heading: a heading error rotates
        // every horizontal command by that error.
        return {rotz(a, -yaw_est), true};
    }
};

}  // namespace dn
