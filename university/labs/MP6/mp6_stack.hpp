// mp6_stack.hpp - MP6 starter lab (M1): the whole simulated stack wired together, the delivery
// task, the log in a fixed schema, and the milestone's acceptance checks.
//   drivers (100 Hz): encoders, gyro    beacons (5 Hz)
//   localisation: EKF predict (100 Hz), update (5 Hz)
//   navigator + task (20 Hz): A* on the inflated map, pure pursuit, stop sequence
//   safety supervisor (100 Hz) -> motor driver -> simulated robot (100 Hz physics)
// It is the university's own code with the SHAPE of a ROS 2 system; no ROS 2 name or API is used.
#pragma once
#include "mp6_ekf.hpp"
#include "mp6_robot.hpp"
#include "mp6_safety.hpp"
#include <cstdio>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

namespace mp6 {

struct Stop { std::string name; Pose pose; double dwell; };

struct Config
{
    std::uint64_t seed = 1;
    Pose dock{1.00, 1.90, 0.00};
    std::vector<Stop> stops;
    std::vector<Landmark> mapPosts;       // where the software believes the posts are
    std::vector<Landmark> truePosts;      // where they really are (same as the map unless moved)
    RobotParams robot;                    // the simulated kit's truth
    double cfgWheelRadius = 0.050;        // the software's calibration values
    double cfgTrack = 0.300;
    int cfgCpr = 1024;
    EkfParams ekf;
    SafetyParams safety;
    double navMaxVel = 0.30;              // m/s the navigator asks for
    double xyTol = 0.10;                  // navigator's goal tolerance (on the ESTIMATE), m
    double yawTol = 0.15;                 // rad
    double taskTol = 0.20;                // task success: TRUE position error at a stop, m
    double timeLimit = 300.0;             // s
    double estopPress = -1.0, estopRelease = -1.0;
    std::vector<double> resets;           // reset button pulses (s)
    double stallAt = -1.0, stallFor = 0.0;   // navigator produces nothing in this window
    std::string mutant = "none";
};

inline Config readConfig(std::istream& in)
{
    Config c;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string k;
        if (!(s >> k) || k[0] == '#') { continue; }
        if (k == "seed") { s >> c.seed; }
        else if (k == "dock") { s >> c.dock.x >> c.dock.y >> c.dock.th; }
        else if (k == "stop") { Stop st; s >> st.name >> st.pose.x >> st.pose.y >> st.pose.th >> st.dwell; c.stops.push_back(st); }
        else if (k == "post") { Landmark l{}; s >> l.id >> l.x >> l.y; c.mapPosts.push_back(l); }
        else if (k == "post_moved") {                       // forensic: the real post is elsewhere
            Landmark l{};
            s >> l.id >> l.x >> l.y;
            c.truePosts.push_back(l);
        }
        else if (k == "true_wheel_radius") { s >> c.robot.wheelRadius; }
        else if (k == "true_track") { s >> c.robot.track; }
        else if (k == "true_gyro_bias") { s >> c.robot.gyroBias; }
        else if (k == "slip_sd") { s >> c.robot.slipSd; }
        else if (k == "wheel_radius") { s >> c.cfgWheelRadius; }
        else if (k == "track") { s >> c.cfgTrack; }
        else if (k == "encoder_cpr") { s >> c.cfgCpr; }
        else if (k == "nis_gate") { s >> c.ekf.nisGate; }
        else if (k == "speed_limit") { s >> c.safety.speedLimit; }
        else if (k == "watchdog") { s >> c.safety.watchdog; }
        else if (k == "nav_max_vel") { s >> c.navMaxVel; }
        else if (k == "xy_goal_tolerance") { s >> c.xyTol; }
        else if (k == "yaw_goal_tolerance") { s >> c.yawTol; }
        else if (k == "task_tolerance") { s >> c.taskTol; }
        else if (k == "time_limit") { s >> c.timeLimit; }
        else if (k == "estop") { s >> c.estopPress >> c.estopRelease; }
        else if (k == "reset") { double t = 0.0; s >> t; c.resets.push_back(t); }
        else if (k == "stall") { s >> c.stallAt >> c.stallFor; }
        else if (k == "mutant") { s >> c.mutant; }
        else { std::printf("config: unknown key '%s' ignored\n", k.c_str()); }
    }
    return c;
}

// Apply a mutant by name: each one is a deliberate fault that the suite must detect.
inline void applyMutant(Config& c)
{
    if (c.mutant == "no_speed_clamp") { c.safety.clamp = false; }
    else if (c.mutant == "no_watchdog") { c.safety.watchdogOn = false; }
    else if (c.mutant == "estop_unlatched") { c.safety.latchEstop = false; }
    else if (c.mutant == "bearing_sign") { c.ekf.bearingSignBug = true; }
    else if (c.mutant == "radius_plus_2pc") { c.cfgWheelRadius *= 1.02; }
}

struct LegResult { std::string name; bool reached = false; double tArrive = 0.0; double trueErr = 0.0; };

struct Report
{
    bool success = false;
    double tEnd = 0.0;
    std::vector<LegResult> legs;
    int collisions = 0;
    double maxSpeed = 0.0;
    double rmsErr = 0.0, maxErr = 0.0, meanNees = 0.0;
    std::vector<LandmarkStats> posts;
    std::vector<double> travel;           // cumulative true distance, one sample per 10 ms
    double tLock = -1.0;                  // first safe stop
    double tZeroCmd = -1.0;               // first time the drive got a zero command after tLock
    double tStopped = -1.0;               // first time both wheels were (nearly) still after tLock
    double stopDist = 0.0;                // distance travelled from tLock to tStopped
    double tReady = -1.0;                 // reset accepted after the first safe stop
    std::vector<std::string> events;
    double travelBetween(double a, double b) const
    {
        auto at = [&](double t) {
            const long i = std::clamp(static_cast<long>(t / 0.01), 0L, static_cast<long>(travel.size()) - 1);
            return travel[static_cast<std::size_t>(i)];
        };
        return travel.empty() ? 0.0 : at(b) - at(a);
    }
};

// The navigator: A* on the inflated static map (F9-54, F9-56), pure pursuit (F9-56).
class Navigator
{
public:
    Navigator(const rb::Grid& map, double maxVel, double xyTol, double yawTol) : map_(map), xyTol_(xyTol), yawTol_(yawTol)
    {
        nc_.robotRadius = 0.22;
        nc_.inflationRadius = 0.60;
        nc_.maxVel = maxVel;
        cg_ = rb::Costmap(map_, nc_).build();
    }
    void setGoal(const Pose& est, const Pose& goal) { goal_ = goal; replan(est); }
    bool replan(const Pose& est)
    {
        rb::Cell s = rb::cellAt(est.x, est.y);
        for (int r = 1; r <= 3 && cg_.at(s.i, s.j) == rb::kLethal; ++r) {     // step out of the inflated zone
            for (int b = -r; b <= r; ++b) {
                for (int a = -r; a <= r; ++a) {
                    if (cg_.at(s.i + a, s.j + b) != rb::kLethal && cg_.at(s.i, s.j) == rb::kLethal) { s = {s.i + a, s.j + b}; }
                }
            }
        }
        const rb::PlanResult pr = rb::plan(cg_, s, rb::cellAt(goal_.x, goal_.y), 1.0);
        path_ = rb::toPoses(pr.path);
        idx_ = 0;
        return !path_.empty();
    }
    // One 20 Hz tick on the ESTIMATED pose. Sets `reached` when the estimate is at the goal.
    Cmd tick(const Pose& est, bool& reached)
    {
        reached = false;
        const double d = std::hypot(goal_.x - est.x, goal_.y - est.y);
        if (d <= xyTol_ || (atGoal_ && d <= 2.0 * xyTol_)) {
            atGoal_ = true;
            const double e = wrapAngle(goal_.th - est.th);
            if (std::fabs(e) <= yawTol_) { reached = true; return {}; }
            return {0.0, std::copysign(std::fmin(nc_.maxRotVel, 2.0 * std::fabs(e)), e)};
        }
        atGoal_ = false;
        if (path_.empty()) { return {}; }
        const rb::Cmd c = rb::purePursuit(est, path_, idx_, nc_);
        return {std::fmin(c.v, std::fmax(0.08, d)), c.w};
    }
    void clearGoal() { atGoal_ = false; }

private:
    const rb::Grid& map_;
    rb::NavConfig nc_;
    rb::CostGrid cg_;
    double xyTol_, yawTol_;
    Pose goal_{};
    std::vector<Pose> path_;
    std::size_t idx_ = 0;
    bool atGoal_ = false;
};

// Run the delivery task once. `log` (may be null) receives the CSV log at 2 Hz;
// `verbose` prints the event log as it happens.
inline Report runMission(Config cfg, std::FILE* log, bool verbose)
{
    applyMutant(cfg);
    Report rep;
    const rb::Grid world = rb::makeHouse();
    const rb::Grid map = rb::makeHouse();
    std::vector<Landmark> posts = cfg.mapPosts;              // the real posts: the map, unless moved
    for (const Landmark& m : cfg.truePosts) {
        for (Landmark& l : posts) { if (l.id == m.id) { l = m; } }
    }
    SimRobot robot(world, cfg.robot, posts, cfg.dock, cfg.seed);
    EkfParams ep = cfg.ekf;
    ep.rangeSd = cfg.robot.rangeSd;
    ep.bearingSd = cfg.robot.bearingSd;
    Ekf ekf(ep, cfg.dock, static_cast<int>(cfg.mapPosts.size()));
    Supervisor sup(cfg.safety);
    Navigator nav(map, cfg.navMaxVel, cfg.xyTol, cfg.yawTol);

    auto event = [&](double t, const std::string& s) {
        char buf[200];
        std::snprintf(buf, sizeof buf, "t=%6.2f %s", t, s.c_str());
        rep.events.push_back(buf);
        if (verbose) { std::printf("%s\n", buf); }
    };
    if (log) { std::fprintf(log, "t,leg,state,x_true,y_true,th_true,x_est,y_est,th_est,sd_x,sd_y,sd_th,v_cmd,w_cmd,v_true\n"); }

    const double dt = 0.01;
    EncoderSample prevEnc = robot.encoders();
    Cmd cmd{};
    double stamp = 0.0;
    std::size_t leg = 0;
    bool dwelling = false;
    double dwellUntil = 0.0;
    double lastPlan = 0.0;
    bool wasLocked = false;
    double se = 0.0, sumNees = 0.0;
    int nErr = 0;
    rep.legs.resize(cfg.stops.size());
    if (!cfg.stops.empty()) {
        nav.setGoal(ekf.pose(), cfg.stops[0].pose);
        event(0.0, "task: leg 1 -> " + cfg.stops[0].name);
    }
    const long steps = static_cast<long>(cfg.timeLimit / dt);
    for (long k = 0; k <= steps; ++k) {
        const double t = k * dt;
        // 1. physics, then the drivers read the simulated hardware
        robot.step(dt);
        const EncoderSample enc = robot.encoders();
        const double kd = 2.0 * kPi * cfg.cfgWheelRadius / cfg.cfgCpr;
        const double v = 0.5 * ((enc.left - prevEnc.left) + (enc.right - prevEnc.right)) * kd / dt;
        prevEnc = enc;
        // 2. localisation
        ekf.predict(v, robot.gyro(), dt);
        if (k % 20 == 0) {
            for (const BeaconReading& z : robot.beacons()) {
                const Landmark& m = cfg.mapPosts[static_cast<std::size_t>(z.id)];
                ekf.update(z, m.x, m.y);
            }
        }
        const Pose est = ekf.pose();
        // 3. navigator and task at 20 Hz (silent while "stalled")
        const bool stalled = cfg.stallAt >= 0.0 && t >= cfg.stallAt && t < cfg.stallAt + cfg.stallFor;
        if (k % 5 == 0 && !stalled && leg < cfg.stops.size()) {
            const bool locked = sup.state() == SafeState::Locked;
            if (wasLocked && !locked) { nav.replan(est); lastPlan = t; }
            wasLocked = locked;
            if (dwelling) {
                cmd = {};
                if (t >= dwellUntil) {
                    dwelling = false;
                    ++leg;
                    if (leg < cfg.stops.size()) {
                        nav.clearGoal();
                        nav.setGoal(est, cfg.stops[leg].pose);
                        lastPlan = t;
                        event(t, "task: leg " + std::to_string(leg + 1) + " -> " + cfg.stops[leg].name);
                    }
                }
            } else {
                if (t - lastPlan >= 2.0) { nav.replan(est); lastPlan = t; }
                bool reached = false;
                cmd = nav.tick(est, reached);
                if (reached) {
                    const Pose& tr = robot.truth();
                    const Stop& st = cfg.stops[leg];
                    LegResult& lr = rep.legs[leg];
                    lr.name = st.name;
                    lr.reached = true;
                    lr.tArrive = t;
                    lr.trueErr = std::hypot(tr.x - st.pose.x, tr.y - st.pose.y);
                    char buf[160];
                    std::snprintf(buf, sizeof buf, "task: at %s (estimate says arrived); true position error %.3f m",
                                  st.name.c_str(), lr.trueErr);
                    event(t, buf);
                    dwelling = true;
                    dwellUntil = t + st.dwell;
                }
            }
            stamp = t;
        } else if (leg >= cfg.stops.size() && k % 5 == 0) {
            cmd = {};
            stamp = t;
        }
        // 4. safety supervisor and motor driver
        const bool estop = cfg.estopPress >= 0.0 && t >= cfg.estopPress && (cfg.estopRelease < 0.0 || t < cfg.estopRelease);
        bool resetPulse = false;
        for (double r : cfg.resets) { if (std::fabs(t - r) < dt / 2) { resetPulse = true; } }
        std::string ev;
        const Cmd out = sup.update(cmd, stamp, estop, resetPulse, t, ev);
        if (!ev.empty()) {
            event(t, "supervisor: " + ev);
            if (rep.tLock < 0.0 && sup.state() == SafeState::Locked) { rep.tLock = t; }
            if (rep.tLock >= 0.0 && rep.tReady < 0.0 && ev.rfind("reset accepted", 0) == 0) { rep.tReady = t; }
            if (rep.tLock >= 0.0 && rep.tReady < 0.0 && ev.rfind("e-stop released", 0) == 0) { rep.tReady = t; }
        }
        if (rep.tLock >= 0.0 && rep.tZeroCmd < 0.0 && out.v == 0.0 && out.w == 0.0) { rep.tZeroCmd = t; }
        robot.setWheelTargets((out.v - 0.5 * out.w * cfg.cfgTrack) / cfg.cfgWheelRadius,
                              (out.v + 0.5 * out.w * cfg.cfgTrack) / cfg.cfgWheelRadius);
        // 5. metrics (simulation only: they use the ground truth) and the log
        rep.travel.push_back(robot.travelled());
        if (rep.tLock >= 0.0 && rep.tStopped < 0.0 && robot.rimSpeedMax() < 0.005) {
            rep.tStopped = t;
            rep.stopDist = rep.travelBetween(rep.tLock, t);
        }
        if (rep.travel.size() > 10) {                        // true speed over a 0.1 s window, as an
            const std::size_t n = rep.travel.size();         // external reference would measure it
            rep.maxSpeed = std::fmax(rep.maxSpeed, (rep.travel[n - 1] - rep.travel[n - 11]) / 0.1);
        }
        const Pose& tr = robot.truth();
        if (k % 50 == 0) {
            const double e = std::hypot(est.x - tr.x, est.y - tr.y);
            if (t >= 5.0) {
                se += e * e;
                rep.maxErr = std::fmax(rep.maxErr, e);
                sumNees += ekf.nees(tr);
                ++nErr;
            }
            if (log) {
                const Mat& P = ekf.cov();
                std::fprintf(log, "%.2f,%zu,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", t,
                             leg + 1, sup.state() == SafeState::Ready ? "READY" : "LOCKED", tr.x, tr.y, tr.th, est.x,
                             est.y, est.th, std::sqrt(P(0, 0)), std::sqrt(P(1, 1)), std::sqrt(P(2, 2)), out.v, out.w,
                             robot.speed());
            }
        }
        if (leg >= cfg.stops.size() && t >= rep.tEnd + 1.0 && rep.tEnd > 0.0) { break; }
        if (leg >= cfg.stops.size() && rep.tEnd == 0.0) {
            rep.tEnd = t;
            event(t, "task: all stops done");
        }
    }
    if (leg < cfg.stops.size()) {
        rep.tEnd = cfg.timeLimit;
        event(cfg.timeLimit, "task: TIME LIMIT reached at leg " + std::to_string(leg + 1));
    }
    rep.collisions = robot.collisions();
    rep.rmsErr = nErr > 0 ? std::sqrt(se / nErr) : 0.0;
    rep.meanNees = nErr > 0 ? sumNees / nErr : 0.0;
    rep.posts = ekf.stats();
    rep.success = leg >= cfg.stops.size() && rep.collisions == 0;
    for (const LegResult& l : rep.legs) { rep.success = rep.success && l.reached && l.trueErr <= cfg.taskTol; }
    return rep;
}

// The milestone's acceptance checks for one nominal run (the lab plan's numbers; see the handbook).
struct Check { std::string id, what; bool pass; std::string value; };

inline std::vector<Check> nominalChecks(const Config& cfg, const Report& r)
{
    char b1[80], b2[80], b3[80], b4[80];
    double worst = 0.0;
    bool all = true;
    for (const LegResult& l : r.legs) {
        all = all && l.reached;
        worst = std::fmax(worst, l.reached ? l.trueErr : 99.0);
    }
    std::snprintf(b1, sizeof b1, "worst true stop error %.3f m, %.1f s", worst, r.tEnd);
    std::snprintf(b2, sizeof b2, "%d contacts", r.collisions);
    std::snprintf(b3, sizeof b3, "RMS %.3f m, max %.3f m, mean NEES %.2f", r.rmsErr, r.maxErr, r.meanNees);
    std::snprintf(b4, sizeof b4, "max true speed %.3f m/s (0.1 s window)", r.maxSpeed);
    return {
        {"T1", "task: every stop reached, true error <= task tolerance", all && worst <= cfg.taskTol && r.tEnd < cfg.timeLimit, b1},
        {"T2", "no bumper contact", r.collisions == 0, b2},
        {"T3", "localisation: RMS <= 0.10 m, max <= 0.30 m, mean NEES <= 6", r.rmsErr <= 0.10 && r.maxErr <= 0.30 && r.meanNees <= 6.0, b3},
        {"T4", "speed never above the limit (+0.01 m/s)", r.maxSpeed <= cfg.safety.speedLimit + 0.01, b4},
    };
}

inline void printPosts(const Config& cfg, const Report& r)
{
    std::printf("post  map x  map y   seen  rejected  mean NIS  max NIS\n");
    for (std::size_t i = 0; i < r.posts.size(); ++i) {
        const LandmarkStats& s = r.posts[i];
        std::printf("L%-3zu %6.2f %6.2f %6d %9d %9.2f %8.1f\n", i, cfg.mapPosts[i].x, cfg.mapPosts[i].y, s.seen,
                    s.rejected, s.seen > 0 ? s.sumNis / s.seen : 0.0, s.maxNis);
    }
}

}  // namespace mp6
