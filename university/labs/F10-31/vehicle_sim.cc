// vehicle_sim.cc - F10-31 Listing 2: the university's mini-SITL. A point-mass "vehicle"
// (system 1, component 1) that speaks U-link over UDP on the loopback interface. It sends a
// heartbeat every second and its position five times a second to every peer it has heard
// from, accepts a mission upload (mission.hpp of F10-30), executes four commands, flies the
// mission in AUTO, and lands if the companion computer's heartbeats stop for 1.5 s while it
// is flying under its control. Command numbers, mode numbers and timeouts are OUR exercise
// values; real SITL, real modes and real failsafe settings are in the chapter's boxes.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "mission.hpp"
#include "udialect.h"
#include "udp.hpp"

namespace {

enum Mode : std::uint32_t { kHold = 0, kGuided = 1, kAuto = 2, kLand = 3 };
const char* modeName(std::uint32_t m)
{
    static const char* names[] = {"HOLD", "GUIDED", "AUTO", "LAND"};
    return m < 4 ? names[m] : "?";
}

struct U {
    using Count = udialect::UMissionCount;
    using Request = udialect::UMissionRequest;
    using ItemMsg = udialect::UMissionItem;
    using Ack = udialect::UMissionAck;
};

bool lookup(std::uint32_t id, std::uint8_t& extra)
{
    const udialect::MsgInfo* i = udialect::find(id);
    if (i == nullptr) {
        return false;
    }
    extra = i->crcExtra;
    return true;
}

using Clock = std::chrono::steady_clock;
const Clock::time_point t0 = Clock::now();
int nowMs()
{
    return static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t0).count());
}

}  // namespace

int main(int argc, char** argv)
{
    const char* portFile = argc > 1 ? argv[1] : "port.txt";
    const int maxMs = argc > 2 ? std::atoi(argv[2]) : 20000;
    UdpSocket sock(0);
    std::ofstream(portFile) << sock.port() << "\n";   // tell the companion where we are
    std::printf("[%5.1f s] VEH  listening on the loopback interface (port written to a file)\n",
                nowMs() / 1000.0);
    std::vector<sockaddr_in> peers;
    std::uint8_t seq = 0;
    auto sendAll = [&](const std::vector<std::uint8_t>& b) {
        for (const auto& p : peers) {
            sock.sendTo(b, p);
        }
    };
    auto status = [&](const char* text) {
        udialect::UStatustext st;
        st.severity = 4;
        std::memcpy(st.text.data(), text, std::strlen(text) < 50 ? std::strlen(text) : 50);
        sendAll(ulink::encodeMsg(st, seq++, 1, 1));
        std::printf("[%5.1f s] VEH  %s\n", nowMs() / 1000.0, text);
    };

    // vehicle state (metres, local frame: x north, y east, z down)
    double x = 0, y = 0, z = 0, tx = 0, ty = 0, tz = 0;
    bool armed = false;
    std::uint32_t mode = kHold;
    std::size_t wp = 0;
    int lastCompanionHb = -1;
    bool failsafeDone = false;
    int landedAt = -1;
    auto setMode = [&](std::uint32_t m, const char* why) {
        std::printf("[%5.1f s] VEH  mode %s -> %s (%s)\n", nowMs() / 1000.0, modeName(mode),
                    modeName(m), why);
        mode = m;
    };

    mission::Receiver<U> receiver(1, [&](std::vector<std::uint8_t> b, const char* what) {
        sendAll(b);
        std::printf("[%5.1f s] VEH  mission -> %s\n", nowMs() / 1000.0, what);
    });
    ulink::Parser parser(lookup);

    int nextHb = 0, nextPos = 0, nextStep = 20;
    while (nowMs() < maxMs) {
        sockaddr_in from{};
        const auto bytes = sock.receive(5, from);
        if (!bytes.empty()) {
            bool known = false;
            for (const auto& p : peers) {
                known = known || (p.sin_port == from.sin_port);
            }
            if (!known) {
                peers.push_back(from);
                std::printf("[%5.1f s] VEH  new peer: will send telemetry to it\n",
                            nowMs() / 1000.0);
            }
        }
        for (std::uint8_t b : bytes) {
            auto f = parser.feed(b);
            if (!f) {
                continue;
            }
            const int now = nowMs();
            if (f->msgid == udialect::UHeartbeat::kId && f->compid == 191) {
                lastCompanionHb = now;
            } else if (f->msgid == udialect::UCommand::kId) {
                const auto c = udialect::UCommand::unpack(f->payload.data(), f->payload.size());
                std::uint8_t result = 0;
                if (c.command == 1) {                                  // ARM
                    armed = c.param1 > 0.5f;
                } else if (c.command == 2 && armed && mode == kGuided) { // TAKEOFF
                    tz = -c.param1;
                } else if (c.command == 3 && c.param1 >= 0 && c.param1 <= 3) {   // SET_MODE
                    if (static_cast<std::uint32_t>(c.param1) == kAuto && receiver.items.empty()) {
                        result = 2;
                    } else {
                        setMode(static_cast<std::uint32_t>(c.param1), "command");
                        if (mode == kAuto) { wp = 0; }
                    }
                } else if (c.command == 4 && mode == kGuided && z < -1.0) {      // GOTO
                    tx = c.param1;
                    ty = c.param2;
                } else {
                    result = c.command <= 4 ? 2 : 3;
                }
                udialect::UCommandAck a;
                a.command = c.command;
                a.result = result;
                sendAll(ulink::encodeMsg(a, seq++, 1, 1));
                std::printf("[%5.1f s] VEH  command %u (try %u) -> %s\n", now / 1000.0, c.command,
                            c.confirmation + 1, result == 0 ? "ACCEPTED" : "DENIED");
            } else {
                receiver.onFrame(*f, now);
            }
        }
        const int now = nowMs();
        receiver.onTick(now);

        // link-loss failsafe: companion heartbeats stopped while flying under its control
        if (!failsafeDone && lastCompanionHb >= 0 && now - lastCompanionHb > 1500 &&
            (mode == kGuided || mode == kAuto) && z < -0.5) {
            failsafeDone = true;
            std::printf("[%5.1f s] VEH  no companion heartbeat for %d ms (last at %.1f s)\n",
                        now / 1000.0, now - lastCompanionHb, lastCompanionHb / 1000.0);
            setMode(kLand, "failsafe: companion link lost");
            status("Failsafe: companion link lost, LAND");
        }

        if (now >= nextStep) {                       // physics and guidance, every 20 ms
            nextStep += 20;
            const double dt = 0.02;
            if (mode == kAuto && wp < receiver.items.size()) {
                const auto& it = receiver.items[wp];
                tx = it.x / 1000.0;
                ty = it.y / 1000.0;
                tz = -it.z;
                if (std::hypot(tx - x, ty - y) < 0.3 && std::fabs(tz - z) < 0.3) {
                    status(("Reached " + it.label).c_str());
                    if (++wp == receiver.items.size()) {
                        status("Mission complete, holding");
                        setMode(kHold, "mission complete");
                    }
                }
            }
            if (mode == kLand) {
                tx = x;
                ty = y;
                tz = 0;
            }
            if (armed) {
                const double dx = tx - x, dy = ty - y, d = std::hypot(dx, dy);
                const double step = std::min(d, 5.0 * dt);
                if (d > 1e-9) {
                    x += dx / d * step;
                    y += dy / d * step;
                }
                const double dz = tz - z;
                z += std::clamp(dz, -2.5 * dt, 2.5 * dt);
                if (mode == kLand && z > -0.01) {
                    z = 0;
                    armed = false;
                    landedAt = now;
                    status("Landed, disarmed");
                }
            }
        }
        if (now >= nextHb) {
            nextHb += 1000;
            udialect::UHeartbeat hb;
            hb.type = 2;
            hb.autopilot = 3;
            hb.base_mode = armed ? 0x80 : 0;
            hb.custom_mode = mode;
            hb.system_status = armed ? 2 : 1;
            sendAll(ulink::encodeMsg(hb, seq++, 1, 1));
        }
        if (now >= nextPos) {
            nextPos += 200;
            udialect::UPosition p;
            p.time_ms = static_cast<std::uint32_t>(now);
            p.x_mm = static_cast<std::int32_t>(std::lround(x * 1000));
            p.y_mm = static_cast<std::int32_t>(std::lround(y * 1000));
            p.z_mm = static_cast<std::int32_t>(std::lround(z * 1000));
            p.quality = 100;
            sendAll(ulink::encodeMsg(p, seq++, 1, 1));
        }
        if (landedAt >= 0 && now - landedAt > 1500) {   // keep reporting briefly, then stop
            break;
        }
    }
    std::printf("[%5.1f s] VEH  exit: frames ok %u, CRC errors %u; final position (%.1f, %.1f, %.1f) m\n",
                nowMs() / 1000.0, parser.stats.framesOk, parser.stats.crcErrors, x, y, -z);
    return 0;
}
