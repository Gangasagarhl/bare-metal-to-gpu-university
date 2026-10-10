// pkt.hh - a binary sensor frame, as a robot's sensor board might send it over a serial line:
//   byte 0-1  magic 'S' 'P'
//   byte 2    type: 1 = range (payload: 2 bytes, millimetres, little-endian), 2 = text message
//   byte 3    payload length N
//   4..4+N-1  payload
//   4+N       checksum: sum of the payload bytes, modulo 256
#pragma once
#include <cstddef>
#include <cstdint>

struct Frame {
    int type = 0;
    unsigned range_mm = 0;
    char text[32] = {};  // a text message is kept as a C string of at most 31 characters
};

// Returns true and fills f if data[0..n) is one valid frame.
bool parse_frame(const std::uint8_t* data, std::size_t n, Frame& f);
