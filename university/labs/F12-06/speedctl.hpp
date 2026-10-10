// speedctl.hpp - the small system under test used in SE302 (F12-06, F12-07, F12-09).
// A delivery robot drives toward a wall. A range sensor sends text packets "R <millimetres>";
// the parser turns a packet into metres, the governor turns a distance into a speed limit,
// and the loop applies the limit to a motor that can only change speed gradually.
#pragma once
#include <charconv>
#include <optional>
#include <string_view>

namespace speedctl {

// Unit 1 - parser. "R 1234" means 1234 mm. Returns metres, or nothing if malformed.
inline std::optional<double> parse_range_m(std::string_view packet)
{
    if (packet.size() < 3 || packet[0] != 'R' || packet[1] != ' ') {
        return std::nullopt;
    }
    unsigned mm = 0;
    const char* first = packet.data() + 2;
    const char* last = packet.data() + packet.size();
    auto [end, ec] = std::from_chars(first, last, mm);
    if (ec != std::errc{} || end != last || mm > 65535) {
        return std::nullopt;
    }
    return mm / 1000.0;
}

// Unit 2 - governor. Speed limit in m/s for a distance d in metres to the obstacle:
// 0 at or below kStop, kVmax at or above kFull, a straight line in between.
constexpr double kStop = 0.5;
constexpr double kFull = 1.0;
constexpr double kVmax = 1.0;

inline double governor(double d)
{
    if (d <= kStop) {
        return 0.0;
    }
    if (d >= kFull) {
        return kVmax;
    }
    return kVmax * (d - kStop) / (kFull - kStop);
}

}  // namespace speedctl
