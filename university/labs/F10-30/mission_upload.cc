// mission_upload.cc - F10-30 Listing 6: upload a three-item mission from a ground station
// (system 255) to a vehicle (system 1) over a simulated link with 40 ms delay. Run A loses
// nothing; run B loses one item on the way up and the final acknowledgement on the way down.
// Every message really goes through ulink::encode and a byte-by-byte ulink::Parser.
#include <cstdio>
#include <vector>

#include "mission.hpp"
#include "udialect.h"

namespace {

struct U {   // the dialect bundle used by both sides here
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

void run(const char* title, std::vector<int> dropUp, std::vector<int> dropDown)
{
    std::printf("%s\n", title);
    mission::Pipe up, down;          // up: GCS -> vehicle, down: vehicle -> GCS
    up.drop = std::move(dropUp);
    down.drop = std::move(dropDown);
    int now = 0;
    const std::vector<mission::Item> plan = {
        {16, 0, 0, 10.0f, "TAKEOFF"}, {16, 20000, 0, 15.0f, "WP1"}, {16, 20000, 20000, 15.0f, "WP2"}};
    mission::Sender<U> gcs(plan, 255, 1, [&](std::vector<std::uint8_t> b, const char* what) {
        const bool ok = up.push(std::move(b), now);
        std::printf("%6d ms  GCS  -> %s%s\n", now, what, ok ? "" : "   (LOST on the link)");
    });
    mission::Receiver<U> veh(1, [&](std::vector<std::uint8_t> b, const char* what) {
        const bool ok = down.push(std::move(b), now);
        std::printf("%6d ms  VEH  -> %s%s\n", now, what, ok ? "" : "   (LOST on the link)");
    });
    ulink::Parser gcsRx(lookup), vehRx(lookup);
    gcs.start(now);
    for (now = 1; now <= 5000; ++now) {
        up.deliver(now, [&](std::uint8_t b) {
            if (auto f = vehRx.feed(b)) { veh.onFrame(*f, now); }
        });
        down.deliver(now, [&](std::uint8_t b) {
            if (auto f = gcsRx.feed(b)) { gcs.onFrame(*f, now); }
        });
        gcs.onTick(now);
        veh.onTick(now);
        if (gcs.state != mission::Sender<U>::State::Sending &&
            veh.state != mission::Receiver<U>::State::Receiving && up.queue.empty() &&
            down.queue.empty()) {
            break;
        }
    }
    std::printf("vehicle stored %zu item(s):", veh.items.size());
    for (const auto& it : veh.items) {
        std::printf(" %s(%d,%d,%.1f)", it.label.c_str(), it.x, it.y, static_cast<double>(it.z));
    }
    std::printf("\nframes sent up %d, down %d; vehicle parser: %u ok, %u CRC errors\n\n", up.sent,
                down.sent, vehRx.stats.framesOk, vehRx.stats.crcErrors);
}

}  // namespace

int main()
{
    run("Run A: perfect link", {}, {});
    run("Run B: the 3rd frame up (ITEM 1) and the 5th frame down (the ACK) are lost", {3}, {5});
    return 0;
}
