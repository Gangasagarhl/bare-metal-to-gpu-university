// companion.cc - F10-31 Listing 3: a companion-computer program (system 1, component 191)
// that speaks U-link to the mini-SITL over UDP: it sends its own heartbeat every second,
// waits for the vehicle's heartbeat, uploads a mission, commands GUIDED, ARM, TAKEOFF and
// AUTO with retries, follows the flight from the position stream, and finally stops its
// heartbeat to test the vehicle's link-loss failsafe. Command and mode numbers are the
// university's exercise values (see vehicle_sim.cc).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "mission.hpp"
#include "udialect.h"
#include "udp.hpp"

namespace {

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

const char* modeName(std::uint32_t m)
{
    static const char* names[] = {"HOLD", "GUIDED", "AUTO", "LAND"};
    return m < 4 ? names[m] : "?";
}

class Companion {
public:
    Companion(std::uint16_t vehiclePort) : vehicle_(loopback(vehiclePort)), parser_(lookup) {}

    // one pass of the event loop: heartbeat out, datagrams in
    void poll(int waitMs)
    {
        const int now = nowMs();
        if (heartbeatOn && now >= nextHb_) {
            nextHb_ = now + 1000;
            udialect::UHeartbeat hb;
            hb.type = 18;                    // our number for "onboard computer"
            hb.system_status = 2;
            send(ulink::encodeMsg(hb, seq_++, 1, 191));
        }
        sockaddr_in from{};
        for (std::uint8_t b : sock_.receive(waitMs, from)) {
            if (auto f = parser_.feed(b)) {
                handle(*f);
            }
        }
        if (uploader_) {
            uploader_->onTick(nowMs());
        }
    }
    void send(const std::vector<std::uint8_t>& b) { sock_.sendTo(b, vehicle_); }

    bool command(std::uint16_t cmd, float p1, float p2, const char* what)
    {
        for (std::uint8_t attempt = 0; attempt < 3; ++attempt) {
            udialect::UCommand c;
            c.target_system = 1;
            c.target_component = 1;
            c.command = cmd;
            c.confirmation = attempt;
            c.param1 = p1;
            c.param2 = p2;
            ackFor_ = -1;
            send(ulink::encodeMsg(c, seq_++, 1, 191));
            const int deadline = nowMs() + 500;
            while (nowMs() < deadline && ackFor_ != cmd) {
                poll(5);
            }
            if (ackFor_ == cmd) {
                std::printf("[%5.1f s] COMP %s: %s\n", nowMs() / 1000.0, what,
                            ackResult_ == 0 ? "accepted" : "DENIED");
                return ackResult_ == 0;
            }
            std::printf("[%5.1f s] COMP %s: no answer, retry\n", nowMs() / 1000.0, what);
        }
        return false;
    }

    bool uploadMission(const std::vector<mission::Item>& items)
    {
        uploader_ = std::make_unique<mission::Sender<U>>(
            items, 1, 1, [this](std::vector<std::uint8_t> b, const char* what) {
                send(b);
                std::printf("[%5.1f s] COMP mission -> %s\n", nowMs() / 1000.0, what);
            });
        uploader_->start(nowMs());
        while (uploader_->state == mission::Sender<U>::State::Sending) {
            poll(5);
        }
        const bool ok = uploader_->state == mission::Sender<U>::State::Done;
        uploader_.reset();
        return ok;
    }

    bool heartbeatOn = true;
    bool vehicleSeen = false;
    std::uint32_t vehicleMode = 0;
    bool vehicleArmed = false;
    double altitude = 0;
    std::string lastText;

private:
    void handle(const ulink::Frame& f)
    {
        if (f.msgid == udialect::UHeartbeat::kId && f.compid == 1) {
            const auto hb = udialect::UHeartbeat::unpack(f.payload.data(), f.payload.size());
            if (!vehicleSeen || hb.custom_mode != vehicleMode) {
                std::printf("[%5.1f s] COMP vehicle heartbeat: mode %s, %s\n", nowMs() / 1000.0,
                            modeName(hb.custom_mode), (hb.base_mode & 0x80) ? "armed" : "disarmed");
            }
            vehicleSeen = true;
            vehicleMode = hb.custom_mode;
            vehicleArmed = (hb.base_mode & 0x80) != 0;
        } else if (f.msgid == udialect::UPosition::kId) {
            const auto p = udialect::UPosition::unpack(f.payload.data(), f.payload.size());
            altitude = -p.z_mm / 1000.0;
        } else if (f.msgid == udialect::UCommandAck::kId) {
            const auto a = udialect::UCommandAck::unpack(f.payload.data(), f.payload.size());
            ackFor_ = a.command;
            ackResult_ = a.result;
        } else if (f.msgid == udialect::UStatustext::kId) {
            const auto t = udialect::UStatustext::unpack(f.payload.data(), f.payload.size());
            lastText.assign(t.text.data(), strnlen(t.text.data(), 50));
            std::printf("[%5.1f s] COMP vehicle says: %s\n", nowMs() / 1000.0, lastText.c_str());
        } else if (uploader_) {
            uploader_->onFrame(f, nowMs());
        }
    }

    UdpSocket sock_{0};
    sockaddr_in vehicle_;
    ulink::Parser parser_;
    std::unique_ptr<mission::Sender<U>> uploader_;
    std::uint8_t seq_ = 0;
    int nextHb_ = 0;
    int ackFor_ = -1;
    int ackResult_ = 0;
};

template <class F>
bool waitFor(Companion& c, F&& done, int timeoutMs)
{
    const int deadline = nowMs() + timeoutMs;
    while (nowMs() < deadline) {
        c.poll(5);
        if (done()) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv)
{
    int port = 0;
    std::ifstream(argc > 1 ? argv[1] : "port.txt") >> port;
    Companion c(static_cast<std::uint16_t>(port));
    std::printf("[%5.1f s] COMP started, sending heartbeats\n", nowMs() / 1000.0);
    if (!waitFor(c, [&] { return c.vehicleSeen; }, 3000)) {
        std::printf("COMP no vehicle heartbeat within 3 s: is the simulator running?\n");
        return 1;
    }
    const std::vector<mission::Item> plan = {{16, 10000, 0, 5.0f, "WP1"},
                                             {16, 10000, 10000, 5.0f, "WP2"}};
    if (!c.uploadMission(plan)) {
        return 1;
    }
    c.command(4, 10, 0, "GOTO 10 m north while still on the ground");
    if (!c.command(3, 1, 0, "SET_MODE GUIDED") || !c.command(1, 1, 0, "ARM") ||
        !c.command(2, 5, 0, "TAKEOFF 5 m")) {
        return 1;
    }
    waitFor(c, [&] { return c.altitude > 4.8; }, 6000);
    std::printf("[%5.1f s] COMP altitude %.1f m reached\n", nowMs() / 1000.0, c.altitude);
    c.command(3, 2, 0, "SET_MODE AUTO");
    waitFor(c, [&] { return c.lastText.rfind("Reached WP1", 0) == 0; }, 8000);
    std::printf("[%5.1f s] COMP simulating a crash of my heartbeat thread\n", nowMs() / 1000.0);
    c.heartbeatOn = false;
    waitFor(c, [&] { return !c.vehicleArmed; }, 10000);
    std::printf("[%5.1f s] COMP vehicle is %s in mode %s, altitude %.1f m\n", nowMs() / 1000.0,
                c.vehicleArmed ? "ARMED" : "disarmed", modeName(c.vehicleMode), c.altitude);
    return c.vehicleArmed ? 1 : 0;
}
