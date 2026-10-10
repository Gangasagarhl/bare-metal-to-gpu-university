// forensic_mission.cc - F10-30 forensic evidence generator: the same upload as Listing 6,
// run A (no losses), but the ground station is built from udialect_gcs.xml (namespace
// gcsdialect) while the vehicle is built from udialect.xml. SYNTHETIC evidence from our own
// programs; how it was made is in the answer key.
#include <cstdio>
#include <vector>

#include "gcsdialect.h"
#include "mission.hpp"
#include "udialect.h"

namespace {

struct Veh {
    using Count = udialect::UMissionCount;
    using Request = udialect::UMissionRequest;
    using ItemMsg = udialect::UMissionItem;
    using Ack = udialect::UMissionAck;
};
struct Gcs {
    using Count = gcsdialect::UMissionCount;
    using Request = gcsdialect::UMissionRequest;
    using ItemMsg = gcsdialect::UMissionItem;
    using Ack = gcsdialect::UMissionAck;
};

template <class Info>
bool lookupIn(const Info* i, std::uint8_t& extra)
{
    if (i == nullptr) {
        return false;
    }
    extra = i->crcExtra;
    return true;
}

}  // namespace

int main()
{
    std::printf("Ground station log and vehicle link statistics, mission upload of 3 items\n");
    mission::Pipe up, down;
    int now = 0;
    const std::vector<mission::Item> plan = {
        {16, 0, 0, 10.0f, "TAKEOFF"}, {16, 20000, 0, 15.0f, "WP1"}, {16, 20000, 20000, 15.0f, "WP2"}};
    mission::Sender<Gcs> gcs(plan, 255, 1, [&](std::vector<std::uint8_t> b, const char* what) {
        up.push(std::move(b), now);
        std::printf("%6d ms  GCS  -> %s\n", now, what);
    });
    mission::Receiver<Veh> veh(1, [&](std::vector<std::uint8_t> b, const char* what) {
        down.push(std::move(b), now);
        std::printf("%6d ms  VEH  -> %s\n", now, what);
    });
    ulink::Parser gcsRx([](std::uint32_t id, std::uint8_t& e) { return lookupIn(gcsdialect::find(id), e); });
    ulink::Parser vehRx([](std::uint32_t id, std::uint8_t& e) { return lookupIn(udialect::find(id), e); });
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
        if (gcs.state != mission::Sender<Gcs>::State::Sending &&
            veh.state != mission::Receiver<Veh>::State::Receiving && up.queue.empty() &&
            down.queue.empty()) {
            break;
        }
    }
    std::printf("vehicle link statistics: frames ok %u, CRC errors %u (last bad message id %u), "
                "unknown ids %u\n",
                vehRx.stats.framesOk, vehRx.stats.crcErrors, vehRx.lastBadId, vehRx.stats.unknownId);
    std::printf("ground station link statistics: frames ok %u, CRC errors %u\n",
                gcsRx.stats.framesOk, gcsRx.stats.crcErrors);
    std::printf("vehicle stored %zu item(s)\n", veh.items.size());
    return 0;
}
