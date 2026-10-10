// skeleton.cpp - MP8 starter lab: the walking skeleton of the running example.
// One request travels the whole path: simulated sensor -> perception (CPU gather backend,
// the stub behind contract C-grid that the GPU kernel will replace) -> C-grid v1 frame ->
// controller -> drive, while an independent ground-truth check measures clearance.
// Time is SIMULATED (integer milliseconds); nothing here is a measurement of real hardware.
// Scenario keys (stdin, one per line, '#' comments):
//   end_ms N | period_ms N | delay_ms N | max_age_ms N | freeze_at_ms N (perception stops)
//   box_at_ms N x0 y0 x1 y1 (an obstacle appears) | consumer_checks_age yes|no
//   speed_mm_s N | decel_mm_s2 N | lookahead_mm N | goal_x_mm N
#include "controller.hpp"
#include "perception.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Scenario
{
    int endMs = 8000;
    int periodMs = 100;
    int delayMs = 30;
    int maxAgeMs = 250;
    int freezeAtMs = -1;           // -1: perception never stops
    int boxAtMs = -1;              // -1: no extra obstacle
    rb::Rect box{0, 0, 0, 0};
    bool checksAge = true;
    int speedMmS = 400;
    int decelMmS2 = 2000;
    int lookaheadMm = 500;
    int goalXMm = 5300;
};

constexpr double kRobotRadius = 0.20;   // metres; inflation uses 0.25 (5 cm margin)
constexpr double kInflate = 0.25;
constexpr double kY = 1.65;             // the robot drives along y = 1.65 m, heading +x
constexpr int kStepMs = 10;             // control period

// Clearance from the robot's disc to the nearest occupied cell (metres; <= 0 = contact).
double clearance(const rb::Grid& g, double x, double y)
{
    double best = std::numeric_limits<double>::infinity();
    for (int j = 0; j < rb::kH; ++j) {
        for (int i = 0; i < rb::kW; ++i) {
            if (!g.occ(i, j)) {
                continue;
            }
            const double cx = std::clamp(x, i * rb::kCell, (i + 1) * rb::kCell);
            const double cy = std::clamp(y, j * rb::kCell, (j + 1) * rb::kCell);
            best = std::min(best, std::hypot(x - cx, y - cy));
        }
    }
    return best - kRobotRadius;
}

Scenario readScenario()
{
    Scenario s;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string key;
        if (!(in >> key) || key[0] == '#') {
            continue;
        }
        if (key == "end_ms") { in >> s.endMs; }
        else if (key == "period_ms") { in >> s.periodMs; }
        else if (key == "delay_ms") { in >> s.delayMs; }
        else if (key == "max_age_ms") { in >> s.maxAgeMs; }
        else if (key == "freeze_at_ms") { in >> s.freezeAtMs; }
        else if (key == "box_at_ms") { in >> s.boxAtMs >> s.box.x0 >> s.box.y0 >> s.box.x1 >> s.box.y1; }
        else if (key == "consumer_checks_age") { std::string v; in >> v; s.checksAge = (v == "yes"); }
        else if (key == "speed_mm_s") { in >> s.speedMmS; }
        else if (key == "decel_mm_s2") { in >> s.decelMmS2; }
        else if (key == "lookahead_mm") { in >> s.lookaheadMm; }
        else if (key == "goal_x_mm") { in >> s.goalXMm; }
        else { std::printf("unknown key '%s' (counted as a failure)\n", key.c_str()); s.endMs = -1; }
    }
    return s;
}

}  // namespace

int main()
{
    const Scenario s = readScenario();
    if (s.endMs < 0) {
        return 1;
    }
    std::printf("scenario: period %d ms, delay %d ms, max age %d ms, freeze %d ms, box %d ms, age check %s\n",
                s.periodMs, s.delayMs, s.maxAgeMs, s.freezeAtMs, s.boxAtMs, s.checksAge ? "yes" : "no");

    const rb::Grid plain = rb::makeHouse();
    const rb::Grid withBox = rb::makeHouse({s.box});
    const mp8::Expect expect{rb::kW, rb::kH, 100, s.maxAgeMs};

    struct Pending { int publishMs; mp8::GridMsg msg; };
    std::vector<Pending> inFlight;
    bool haveFrame = false;
    mp8::GridMsg last;
    std::uint32_t lastSeq = 0;
    std::uint32_t nextSeq = 1;
    int lastStampMs = -1;

    double x = 0.80;
    double v = 0.0;                      // m/s
    std::string mode = "WAIT";          // until the first usable frame arrives
    double minClear = std::numeric_limits<double>::infinity();
    int contactMs = -1;
    int stoppedMs = -1;                  // first time v == 0 after having moved
    int rejected = 0;
    bool moved = false;

    for (int t = 0; t <= s.endMs; t += kStepMs) {
        const bool boxNow = s.boxAtMs >= 0 && t >= s.boxAtMs;
        if (s.boxAtMs >= 0 && t == s.boxAtMs) {
            std::printf("t=%5d  world: box appears at x %.2f-%.2f m\n", t, s.box.x0, s.box.x1);
        }
        // Perception: the sensor samples every period; the frame is published delay ms later.
        if (t % s.periodMs == 0) {
            if (s.freezeAtMs >= 0 && t >= s.freezeAtMs) {
                if (t == s.freezeAtMs) {
                    std::printf("t=%5d  perception: no more frames (injected fault: kernel hang)\n", t);
                }
            } else {
                const rb::Grid& world = boxNow ? withBox : plain;
                inFlight.push_back({t + s.delayMs,
                                    mp8::makeFrame(nextSeq++, t, mp8::inflateGatherCpu(mp8::occBytes(world), kInflate))});
            }
        }
        // Controller: take every frame published by now, oldest first.
        while (!inFlight.empty() && inFlight.front().publishMs <= t) {
            const mp8::GridMsg m = inFlight.front().msg;
            inFlight.erase(inFlight.begin());
            const std::string why = s.checksAge ? mp8::check(m, expect, lastSeq, t)
                                                : mp8::check(m, mp8::Expect{rb::kW, rb::kH, 100, 1 << 30}, lastSeq, t);
            if (why.empty()) {
                last = m;
                lastSeq = m.seq;
                lastStampMs = static_cast<int>(m.stampMs);
                haveFrame = true;
            } else {
                ++rejected;
                std::printf("t=%5d  controller: frame %u rejected: %s\n", t, m.seq, why.c_str());
            }
        }
        const bool fresh = haveFrame && (!s.checksAge || t - lastStampMs <= s.maxAgeMs);
        std::string want;
        if (!fresh) {
            want = moved ? "SAFE_STOP" : "WAIT";
        } else if (mp8::pathBlocked(last, x, kY, s.lookaheadMm / 1000.0)) {
            want = "OBSTACLE_STOP";
        } else if (x * 1000.0 >= s.goalXMm) {
            want = "GOAL_STOP";
        } else {
            want = "DRIVE";
        }
        if (mode == "SAFE_STOP") {
            want = "SAFE_STOP";          // latched: only an operator reset leaves it (F9-69)
        }
        if (want != mode) {
            std::printf("t=%5d  controller: %s -> %s at x %.3f m, v %.3f m/s", t, mode.c_str(), want.c_str(), x, v);
            if (haveFrame) {
                std::printf(" (frame %u, age %d ms)", last.seq, t - lastStampMs);
            }
            std::printf("\n");
            mode = want;
        }
        // Drive: accelerate or brake at the deceleration limit (same limit both ways).
        const double a = s.decelMmS2 / 1000.0;
        const double dt = kStepMs / 1000.0;
        const double target = (mode == "DRIVE") ? s.speedMmS / 1000.0 : 0.0;
        v = (v < target) ? std::min(target, v + a * dt) : std::max(target, v - a * dt);
        x += v * dt;
        moved = moved || v > 0.0;
        if (moved && v == 0.0 && stoppedMs < 0) {
            stoppedMs = t;
        }
        if (v > 0.0) {
            stoppedMs = -1;
        }
        const double c = clearance(boxNow ? withBox : plain, x, kY);
        minClear = std::min(minClear, c);
        if (c <= 0.0 && contactMs < 0) {
            contactMs = t;
            std::printf("t=%5d  GROUND TRUTH: contact at x %.3f m, v %.3f m/s\n", t, x, v);
            break;
        }
    }

    std::printf("end: x %.3f m, v %.3f m/s, mode %s, frames rejected %d\n", x, v, mode.c_str(), rejected);
    std::printf("minimum clearance (ground truth): %.3f m\n", minClear);
    int failures = 0;
    const bool a1 = contactMs < 0 && minClear > 0.0;
    std::printf("A1 no_contact_with_any_obstacle: %s\n", a1 ? "pass" : "FAIL");
    failures += a1 ? 0 : 1;
    if (s.freezeAtMs >= 0) {
        // Budget: the last frame may be max_age old, plus one control step, plus the braking time.
        const int lastStampBeforeFreeze = ((s.freezeAtMs - 1) / s.periodMs) * s.periodMs;
        const int budget = lastStampBeforeFreeze + s.maxAgeMs + kStepMs +
                           static_cast<int>(std::ceil(1000.0 * s.speedMmS / s.decelMmS2));
        const bool a2 = stoppedMs >= 0 && stoppedMs <= budget;
        std::printf("A2 stopped_within_budget_after_perception_loss: %s (stopped at %d ms, budget %d ms)\n",
                    a2 ? "pass" : "FAIL", stoppedMs, budget);
        failures += a2 ? 0 : 1;
    }
    const bool a3 = stoppedMs >= 0 && mode != "DRIVE";
    std::printf("A3 robot_at_rest_at_end: %s (mode %s)\n", a3 ? "pass" : "FAIL", mode.c_str());
    failures += a3 ? 0 : 1;
    std::printf("result: %s\n", failures == 0 ? "all checks pass" : "CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}
