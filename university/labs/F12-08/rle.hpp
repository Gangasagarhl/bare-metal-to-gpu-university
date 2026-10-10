// rle.hpp - run-length encoding of bytes: each run becomes a (count, value) pair, both bytes.
// encode_v1 is the first version (it has a bug); encode is the fixed version.
#pragma once
#include <cstdint>
#include <utility>
#include <vector>

namespace rle {

using Bytes = std::vector<std::uint8_t>;
using Pairs = std::vector<std::pair<std::uint8_t, std::uint8_t>>;  // (count, value)

inline Pairs encode_v1(const Bytes& in)
{
    Pairs out;
    for (std::uint8_t b : in) {
        if (!out.empty() && out.back().second == b) {
            ++out.back().first;  // a count is one byte: what happens at 255 + 1?
        } else {
            out.push_back({1, b});
        }
    }
    return out;
}

inline Pairs encode(const Bytes& in)
{
    Pairs out;
    for (std::uint8_t b : in) {
        if (!out.empty() && out.back().second == b && out.back().first < 255) {
            ++out.back().first;
        } else {
            out.push_back({1, b});  // new value, or the current run is full: start a new pair
        }
    }
    return out;
}

inline Bytes decode(const Pairs& in)
{
    Bytes out;
    for (auto [count, value] : in) {
        out.insert(out.end(), count, value);
    }
    return out;
}

}  // namespace rle
