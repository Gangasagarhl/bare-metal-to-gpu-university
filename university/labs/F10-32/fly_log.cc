// fly_log.cc - F10-32 Listing 2: a short simulated survey flight (at most 60 s) that writes a
// UDF log.
// usage: fly_log <log file> normal|glitch
// "glitch" makes the GNSS receiver lose its fix at t = 24 s (forensic lab). Kinematics,
// attitude, battery and estimator rules are exercise models, not a real vehicle.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "udf.hpp"

namespace {

enum Type : std::uint8_t { PARM = 129, MODE, ATT, GPS, POS, BAT, ERR, MSG };
enum Mode : std::uint8_t { kHold = 0, kGuided = 1, kAuto = 2, kLand = 3 };

struct Noise {                         // deterministic pseudo-random noise in [-1, 1]
    std::uint32_t s = 2024u;
    double next()
    {
        s = s * 1664525u + 1013904223u;
        return (s >> 8) / 8388608.0 - 1.0;
    }
};

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: fly_log <file> normal|glitch\n");
        return 2;
    }
    const bool glitch = std::string(argv[2]) == "glitch";
    udf::Writer log(argv[1]);
    log.define(PARM, "PARM", "QNf", "TimeUS,Name,Value");
    log.define(MODE, "MODE", "QBB", "TimeUS,Mode,Reason");
    log.define(ATT, "ATT", "Qfff", "TimeUS,Roll,Pitch,Yaw");
    log.define(GPS, "GPS", "QBBfff", "TimeUS,Status,NSats,HDop,X,Y");
    log.define(POS, "POS", "QfffB", "TimeUS,X,Y,Alt,PosOK");
    log.define(BAT, "BAT", "Qff", "TimeUS,Volt,Curr");
    log.define(ERR, "ERR", "QBB", "TimeUS,Subsys,Code");
    log.define(MSG, "MSG", "QN", "TimeUS,Message");

    auto rec = [&](Type t, std::uint64_t us) {
        udf::Writer::Record r(t);
        r.raw(us);
        return r;
    };
    auto text = [&](std::uint64_t us, const char* s) {
        auto r = rec(MSG, us);
        r.chars(s, 16);
        log.write(r);
    };
    const char* params[] = {"U_GPS_MINSATS", "U_FS_GPS_ACT", "U_BATT_LOW", "U_WPNAV_SPEED"};
    const float values[] = {6, 3, 14.0f, 5.0f};
    for (int i = 0; i < 4; ++i) {
        auto r = rec(PARM, 0);
        r.chars(params[i], 16);
        r.raw(values[i]);
        log.write(r);
    }

    const std::vector<std::array<double, 3>> wps = {{0, 0, 10}, {30, 0, 10}, {30, 30, 10}, {0, 0, 10}};
    double x = 0, y = 0, alt = 0, vx = 0, vy = 0, yaw = 0;
    Mode mode = kHold;
    bool armed = false, posOk = true, landed = false;
    std::size_t wp = 0;
    int noFixMs = 0;
    Noise n;
    auto setMode = [&](Mode m, std::uint8_t reason, std::uint64_t us) {
        mode = m;
        auto r = rec(MODE, us);
        r.u8(m);
        r.u8(reason);
        log.write(r);
    };
    for (int ms = 0; ms <= 60000; ms += 20) {
        const std::uint64_t us = static_cast<std::uint64_t>(ms) * 1000;
        if (ms == 2000) {
            armed = true;
            text(us, "Armed");
        }
        if (ms == 3000) {
            setMode(kAuto, 0, us);
        }
        // GNSS receiver model
        const bool lostFix = glitch && ms >= 24000 && ms < 40000;
        int sats = 14 + static_cast<int>(std::lround(n.next()));
        if (lostFix) {
            sats = std::max(2, 13 - (ms - 24000) / 150);
        }
        const std::uint8_t status = sats >= 6 ? 3 : 1;
        noFixMs = status == 3 ? 0 : noFixMs + 20;
        if (posOk && noFixMs >= 1000) {               // estimator gives up after 1 s without fix
            posOk = false;
            auto e = rec(ERR, us);
            e.u8(11);                                 // our subsystem number for "GPS"
            e.u8(2);                                  // our code for "position lost"
            log.write(e);
            text(us, "Position lost");
            if (mode == kAuto) {
                setMode(kLand, 2, us);                // reason 2: failsafe GPS
            }
        }
        // guidance
        double tx = x, ty = y, talt = alt;
        if (mode == kAuto && wp < wps.size()) {
            tx = wps[wp][0];
            ty = wps[wp][1];
            talt = wps[wp][2];
            if (std::hypot(tx - x, ty - y) < 0.3 && std::fabs(talt - alt) < 0.3) {
                ++wp;
                if (wp == wps.size()) {
                    setMode(kLand, 1, us);            // reason 1: mission end
                }
            }
        }
        if (mode == kLand) {
            talt = 0;
            tx = x;
            ty = y;
        }
        if (armed && !landed) {
            const double dx = tx - x, dy = ty - y, d = std::hypot(dx, dy);
            const double speed = std::min(5.0, d);
            const double wantVx = d > 1e-6 ? dx / d * speed : 0, wantVy = d > 1e-6 ? dy / d * speed : 0;
            vx += std::clamp(wantVx - vx, -0.08, 0.08);
            vy += std::clamp(wantVy - vy, -0.08, 0.08);
            if (!posOk) {                             // no position: wind drift is not corrected
                vx = 0.6;
                vy = 0.2;
            }
            x += vx * 0.02;
            y += vy * 0.02;
            alt += std::clamp(talt - alt, -0.05, 0.05);
            if (std::hypot(vx, vy) > 0.5) {
                yaw = std::atan2(vy, vx) * 180.0 / M_PI;
            }
            if (mode == kLand && alt < 0.02) {
                alt = 0;
                landed = true;
                armed = false;
                text(us, "Landed");
            }
        }
        if (ms % 100 == 0) {
            auto r = rec(ATT, us);
            r.raw(static_cast<float>(armed ? 2.0 * vy + 0.4 * n.next() : 0));
            r.raw(static_cast<float>(armed ? -2.0 * vx + 0.4 * n.next() : 0));
            r.raw(static_cast<float>(yaw));
            log.write(r);
        }
        if (ms % 200 == 0) {
            auto g = rec(GPS, us);
            g.u8(status);
            g.u8(static_cast<std::uint8_t>(sats));
            g.raw(static_cast<float>(status == 3 ? 0.8 + 0.05 * n.next() : 9.9));
            g.raw(static_cast<float>(status == 3 ? x + 0.3 * n.next() : 0));
            g.raw(static_cast<float>(status == 3 ? y + 0.3 * n.next() : 0));
            log.write(g);
            auto p = rec(POS, us);
            p.raw(static_cast<float>(x));
            p.raw(static_cast<float>(y));
            p.raw(static_cast<float>(alt));
            p.u8(posOk ? 1 : 0);
            log.write(p);
        }
        if (ms % 1000 == 0) {
            const bool climbing = armed && alt < 9.7 && mode == kAuto;
            const double curr = armed ? 9.0 + (climbing ? 5.0 : 0) + 1.5 * std::hypot(vx, vy) : 0.5;
            auto b = rec(BAT, us);
            b.raw(static_cast<float>(16.6 - 0.012 * ms / 1000.0 - 0.09 * curr));
            b.raw(static_cast<float>(curr));
            log.write(b);
        }
        if (landed && ms % 1000 == 0 && ms > 0) {
            break;
        }
    }
    std::printf("wrote %zu bytes (%s flight)\n", log.bytesWritten, glitch ? "glitch" : "normal");
    return 0;
}
