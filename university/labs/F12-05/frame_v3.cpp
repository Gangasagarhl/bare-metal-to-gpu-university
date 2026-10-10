// frame_v3.cpp - SE301 F12-05: the parser after the author answered the review of the incident fix.
// Frame: 7E <type> <len> <len payload bytes> <sum>, where sum is
// (type + len + every payload byte) modulo 256. Each stdin line is one test case:
//   ok|reject <bytes in hex>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace frame {

constexpr std::uint8_t kStart = 0x7E;
constexpr std::size_t kMaxPayload = 32;          // firmware 2.0 frames carry up to 32 bytes
static_assert(kMaxPayload <= 255, "the len field is one byte");

struct Frame
{
    std::uint8_t type = 0;
    std::size_t len = 0;
    std::array<std::uint8_t, kMaxPayload> payload{};   // review C1: one constant, not two
};

// Returns false, and leaves out unchanged, for anything malformed.
bool parse(const std::vector<std::uint8_t>& in, Frame& out)
{
    if (in.size() < 4 || in[0] != kStart) {
        return false;
    }
    const std::size_t len = in[2];
    if (len > kMaxPayload || in.size() != len + 4) {
        return false;
    }
    Frame f;
    f.type = in[1];
    f.len = len;
    auto sum = static_cast<std::uint8_t>(in[1] + in[2]);
    for (std::size_t i = 0; i < len; ++i) {
        f.payload[i] = in[3 + i];
        sum = static_cast<std::uint8_t>(sum + in[3 + i]);
    }
    if (sum != in[3 + len]) {
        return false;
    }
    out = f;
    return true;
}

}  // namespace frame

int main()
{
    std::string line;
    int cases = 0;
    int failed = 0;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string expect;
        in >> expect;
        std::vector<std::uint8_t> bytes;
        unsigned v = 0;
        while (in >> std::hex >> v) {
            bytes.push_back(static_cast<std::uint8_t>(v));
        }
        frame::Frame f;
        const bool ok = frame::parse(bytes, f);
        const bool pass = ok == (expect == "ok");
        std::printf("case %2d: %2zu bytes, len field %3u, expect %-6s -> %-6s %s\n", ++cases,
                    bytes.size(), bytes.size() > 2 ? unsigned{bytes[2]} : 0u, expect.c_str(),
                    ok ? "ok" : "reject", pass ? "pass" : "FAIL");
        failed += pass ? 0 : 1;
    }
    std::printf("%d cases, %d failed\n", cases, failed);
    return failed == 0 ? 0 : 1;
}
