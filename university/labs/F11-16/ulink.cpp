// F11-16 Listing 2: "ULink", the university's toy telemetry link, unsigned and signed.
// A frame carries a sender id (sysid, compid), an 8-bit sequence number and a message.
// A signed frame adds a link id, a 48-bit timestamp and a 6-byte tag:
//     tag = first 6 bytes of HMAC-SHA-256(key, header | message | linkid | timestamp)
// The receiver accepts a signed frame only if the tag is right AND its timestamp is
// newer than the last one accepted on that (sysid, compid, linkid) stream.
// ULink is NOT MAVLink: it borrows the idea of signing so that it can be run here.
// Ids, key, sizes and times are this course's exercise values.
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "hmac.h"

struct Frame
{
    int sysid = 0, compid = 0, seq = 0;
    std::string msg;
    bool signedFrame = false;
    int linkid = 0;
    std::uint64_t timestamp = 0;
    std::string tag;    // 12 hex digits

    std::string signedPart() const
    {
        return std::to_string(sysid) + "," + std::to_string(compid) + "," + std::to_string(seq) +
               "|" + msg + "|" + std::to_string(linkid) + "|" + std::to_string(timestamp);
    }
};

std::string tagOf(const std::string& key, const Frame& f)
{
    const sec::Digest d = sec::hmacSha256(sec::bytes(key), sec::bytes(f.signedPart()));
    return sec::hex(d.data(), 6);
}

struct Receiver
{
    bool requireSigned = false;
    std::string key;
    std::map<std::tuple<int, int, int>, std::uint64_t> lastTs;

    std::string accept(const Frame& f)
    {
        if (!requireSigned) {
            return "accepted";
        }
        if (!f.signedFrame) {
            return "dropped: unsigned";
        }
        const std::string want = tagOf(key, f);
        if (f.tag.size() != want.size() ||
            !sec::equalTags(reinterpret_cast<const std::uint8_t*>(f.tag.data()),
                            reinterpret_cast<const std::uint8_t*>(want.data()), want.size())) {
            return "dropped: bad tag";
        }
        const auto stream = std::make_tuple(f.sysid, f.compid, f.linkid);
        const auto it = lastTs.find(stream);
        if (it != lastTs.end() && f.timestamp <= it->second) {
            return "dropped: old timestamp";
        }
        lastTs[stream] = f.timestamp;
        return "accepted";
    }
};

int main()
{
    const std::string key = "shared-secret-of-this-drone-and-gcs";
    std::uint64_t clock = 5000000;    // ground station's signing clock, exercise units
    int seq = 0;
    auto gcs = [&](const std::string& msg) {
        Frame f{20, 190, seq++ & 255, msg, true, 1, clock += 1000, ""};
        f.tag = tagOf(key, f);
        return f;
    };
    std::vector<std::pair<const char*, Frame>> air;
    air.push_back({"gcs", gcs("heartbeat")});
    air.push_back({"gcs", gcs("cmd:arm")});
    const Frame captured = gcs("cmd:takeoff");
    air.push_back({"gcs", captured});
    air.push_back({"gcs", gcs("heartbeat")});
    // attacker: claims to be the ground station, unsigned
    air.push_back({"attacker", Frame{20, 190, 4, "cmd:land", false, 0, 0, ""}});
    // attacker: signed-looking frame with a made-up tag and a fresh timestamp
    air.push_back({"attacker",
                   Frame{20, 190, 4, "cmd:land", true, 1, clock + 500, "a1b2c3d4e5f6"}});
    // attacker: copies a real signed frame and only changes the message
    Frame edited = captured;
    edited.msg = "cmd:land";
    air.push_back({"attacker", edited});
    // attacker: replays the captured frame unchanged
    air.push_back({"attacker", captured});
    air.push_back({"gcs", gcs("heartbeat")});

    for (int mode = 0; mode < 2; ++mode) {
        Receiver rx;
        rx.requireSigned = mode == 1;
        rx.key = key;
        std::printf("receiver: %s\n", mode == 0 ? "accepts unsigned frames" : "requires signing");
        int fromAttacker = 0;
        for (const auto& [who, f] : air) {
            const std::string v = rx.accept(f);
            if (v == "accepted" && std::string(who) == "attacker") {
                ++fromAttacker;
            }
            std::printf("  %-8s sys %d comp %d seq %3d %-12s ts %-8llu tag %-12s %s\n", who,
                        f.sysid, f.compid, f.seq, f.msg.c_str(),
                        static_cast<unsigned long long>(f.timestamp),
                        f.signedFrame ? f.tag.c_str() : "-", v.c_str());
        }
        std::printf("  frames from the attacker accepted: %d\n", fromAttacker);
    }
    return 0;
}
