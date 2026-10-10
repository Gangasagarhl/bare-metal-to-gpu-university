// robotlink.h: the command path of the course robot, reduced to what F11-15 needs.
// The teleop laptop sends text frames "seq=N;cmd=...;v=...;w=..." over the robot's
// network; the motor-controller process decides whether to obey. Three policies:
//   Plain    : obey every well-formed frame (no authentication)
//   Mac      : obey only frames whose tag is HMAC-SHA-256(key, body), truncated
//   MacFresh : as Mac, and the sequence number must be larger than the last one obeyed
// The frame format, the key and the 8-byte tag are this course's exercise choices.
#pragma once
#include <cstdio>
#include <string>

#include "hmac.h"

namespace robot {

enum class Policy { Plain, Mac, MacFresh };

constexpr std::size_t kTagBytes = 8;

inline std::string tagFor(const std::string& key, const std::string& body)
{
    const sec::Digest d = sec::hmacSha256(sec::bytes(key), sec::bytes(body));
    return sec::hex(d.data(), kTagBytes);
}

inline std::string frame(const std::string& key, const std::string& body)
{
    return body + "|tag=" + tagFor(key, body);
}

struct Controller
{
    Policy policy = Policy::Plain;
    std::string key;
    long lastSeq = -1;     // kept in RAM only: lost when the controller restarts
    int obeyed = 0;

    void restart()
    {
        lastSeq = -1;
    }

    // returns the verdict text; obeys (counts) when accepted
    std::string receive(const std::string& f)
    {
        const std::size_t bar = f.find("|tag=");
        const std::string body = f.substr(0, bar);
        if (body.rfind("seq=", 0) != 0) {
            return "rejected: malformed";
        }
        const long seq = std::stol(body.substr(4));
        if (policy != Policy::Plain) {
            if (bar == std::string::npos) {
                return "rejected: no tag";
            }
            const std::string got = f.substr(bar + 5);
            const std::string want = tagFor(key, body);
            if (got.size() != want.size() ||
                !sec::equalTags(reinterpret_cast<const std::uint8_t*>(got.data()),
                                reinterpret_cast<const std::uint8_t*>(want.data()), want.size())) {
                return "rejected: bad tag";
            }
            if (policy == Policy::MacFresh && seq <= lastSeq) {
                return "rejected: old seq " + std::to_string(seq) + " <= " +
                       std::to_string(lastSeq);
            }
        }
        lastSeq = seq;
        ++obeyed;
        return "OBEYED";
    }
};

} // namespace robot
