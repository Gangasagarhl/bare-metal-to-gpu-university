// pkt.cc - the frame parser under test. It contains a planted bug (the fuzzing lab finds it).
// This file is built with coverage instrumentation (-fsanitize-coverage=trace-pc) and with
// AddressSanitizer, so the fuzzer can see which code each input reached, and memory errors stop
// the run with a report.
#include "pkt.hh"

#include <cstring>

bool parse_frame(const std::uint8_t* data, std::size_t n, Frame& f)
{
    if (n < 5) {
        return false;
    }
    if (data[0] != 'S') {
        return false;
    }
    if (data[1] != 'P') {
        return false;
    }
    const std::size_t len = data[3];
    if (n < 4 + len + 1) {  // the whole payload and the checksum must be there
        return false;
    }
    const std::uint8_t* payload = data + 4;
    f.type = data[2];
    if (f.type == 1) {
        if (len != 2) {
            return false;
        }
        f.range_mm = payload[0] | (payload[1] << 8);
    } else if (f.type == 2) {
        std::memcpy(f.text, payload, len);  // copies len bytes into a 32-byte array
        f.text[len < sizeof f.text ? len : sizeof f.text - 1] = '\0';
    } else {
        return false;
    }
    unsigned sum = 0;
    for (std::size_t i = 0; i < len; ++i) {
        sum += payload[i];
    }
    return (sum & 0xFF) == data[4 + len];  // the checksum is checked last
}
