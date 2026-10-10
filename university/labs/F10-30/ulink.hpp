// ulink.hpp - F10-30 Listing 3: framing and parsing for the university's U-link.
// Frame layout (all multi-byte values little-endian):
//   [0] 0xFD start marker      [1] payload length     [2] incompatibility flags
//   [3] compatibility flags    [4] sequence number    [5] system id   [6] component id
//   [7..9] message id (24 bit) [10..] payload          then CRC-16 low, CRC-16 high
// The checksum covers bytes 1 .. end of payload, then the message's crc_extra byte.
// Trailing zero bytes of the payload are not sent (at least one byte is kept); the
// receiver fills them back with zeros. This layout is modelled on MAVLink 2 as the chapter
// recalls it; check every detail in the MAVLink Developer Guide before using it as MAVLink.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace ulink {

inline constexpr std::uint8_t kStx = 0xFD;
inline constexpr std::size_t kHeader = 10;   // bytes before the payload
inline constexpr std::size_t kMaxPayload = 255;

inline std::uint16_t crcAccumulate(std::uint8_t byte, std::uint16_t crc)
{
    std::uint8_t tmp = static_cast<std::uint8_t>(byte ^ (crc & 0xFF));
    tmp = static_cast<std::uint8_t>(tmp ^ (tmp << 4));
    return static_cast<std::uint16_t>((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4));
}

inline std::uint16_t crc(const std::uint8_t* p, std::size_t n, std::uint16_t c = 0xFFFF)
{
    for (std::size_t i = 0; i < n; ++i) {
        c = crcAccumulate(p[i], c);
    }
    return c;
}

struct Frame {
    std::uint8_t seq = 0, sysid = 0, compid = 0;
    std::uint32_t msgid = 0;
    std::vector<std::uint8_t> payload;   // as received (possibly truncated)
};

// Build one frame from a full payload of length n.
inline std::vector<std::uint8_t> encode(std::uint8_t seq, std::uint8_t sysid, std::uint8_t compid,
                                        std::uint32_t msgid, const std::uint8_t* payload,
                                        std::size_t n, std::uint8_t crcExtra)
{
    while (n > 1 && payload[n - 1] == 0) {   // payload truncation
        --n;
    }
    std::vector<std::uint8_t> f = {kStx, static_cast<std::uint8_t>(n), 0, 0, seq, sysid, compid,
                                   static_cast<std::uint8_t>(msgid & 0xFF),
                                   static_cast<std::uint8_t>((msgid >> 8) & 0xFF),
                                   static_cast<std::uint8_t>((msgid >> 16) & 0xFF)};
    f.insert(f.end(), payload, payload + n);
    std::uint16_t c = crc(f.data() + 1, f.size() - 1);
    c = crcAccumulate(crcExtra, c);
    f.push_back(static_cast<std::uint8_t>(c & 0xFF));
    f.push_back(static_cast<std::uint8_t>(c >> 8));
    return f;
}

// Encode a generated message type (it provides kId, kCrcExtra, kLen and pack()).
template <class Msg>
std::vector<std::uint8_t> encodeMsg(const Msg& m, std::uint8_t seq, std::uint8_t sysid,
                                    std::uint8_t compid)
{
    std::uint8_t buf[Msg::kLen];
    const std::size_t n = m.pack(buf);
    return encode(seq, sysid, compid, Msg::kId, buf, n, Msg::kCrcExtra);
}

struct Stats {
    unsigned framesOk = 0, crcErrors = 0, unknownId = 0, bytesSkipped = 0;
};

// A byte-at-a-time parser. lookup(msgid, crcExtra&) says whether the id is known and gives
// its crc_extra; without it the checksum of a frame cannot be checked.
class Parser {
public:
    using Lookup = std::function<bool(std::uint32_t, std::uint8_t&)>;
    explicit Parser(Lookup lookup) : lookup_(std::move(lookup)) {}

    std::optional<Frame> feed(std::uint8_t b)
    {
        if (buf_.empty()) {
            if (b != kStx) {
                ++stats.bytesSkipped;          // noise between frames
                return std::nullopt;
            }
        }
        buf_.push_back(b);
        if (buf_.size() < 2) {
            return std::nullopt;
        }
        const std::size_t total = kHeader + buf_[1] + 2;
        if (buf_.size() < total) {
            return std::nullopt;
        }
        // a whole frame is buffered: check it
        const std::uint32_t id = buf_[7] | (buf_[8] << 8) | (static_cast<std::uint32_t>(buf_[9]) << 16);
        std::uint8_t extra = 0;
        std::optional<Frame> out;
        if (!lookup_(id, extra)) {
            ++stats.unknownId;
        } else {
            std::uint16_t c = crc(buf_.data() + 1, total - 3);
            c = crcAccumulate(extra, c);
            if ((c & 0xFF) != buf_[total - 2] || (c >> 8) != buf_[total - 1]) {
                ++stats.crcErrors;
                lastBadId = id;
            } else {
                ++stats.framesOk;
                Frame f;
                f.seq = buf_[4];
                f.sysid = buf_[5];
                f.compid = buf_[6];
                f.msgid = id;
                f.payload.assign(buf_.begin() + kHeader, buf_.begin() + kHeader + buf_[1]);
                out = f;
            }
        }
        buf_.clear();
        return out;
    }
    Stats stats;
    std::uint32_t lastBadId = 0;

private:
    Lookup lookup_;
    std::vector<std::uint8_t> buf_;
};

}  // namespace ulink
