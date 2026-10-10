// F9-14 Listing 1: how full is each bus? Reads one line per message stream:
//   bus name payload_bytes rate_Hz
// and adds up the bits each stream puts on its bus per second, using a simple bit-cost
// model per bus type. Bus speeds, protocol overheads and all streams are PRETEND values.
// The overhead constants are NOT VERIFIED here (see the chapter's unverified box).
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>

#include "can_model.h"

double bitsPerMessage(const std::string& bus, int bytes)
{
    if (bus == "can") {  // split into frames of at most 8 data bytes
        const int frames = (bytes + CAN_MAX_DLEN - 1) / CAN_MAX_DLEN;
        const int last = bytes - (frames - 1) * CAN_MAX_DLEN;
        return (frames - 1) * canFrameBitsWorst(CAN_MAX_DLEN) + canFrameBitsWorst(last);
    }
    if (bus == "uart") {  // 8N1: 10 bit times per byte, plus a 6-byte header and checksum
        return 10.0 * (bytes + 6);
    }
    if (bus == "i2c") {  // register read: address+W, register, repeated START, address+R, data;
        return 9.0 * (3 + bytes) + 3.0;  // 9 clocks per byte (8 bits + ACK) + START/rSTART/STOP
    }
    if (bus == "eth") {  // UDP over IPv4 over Ethernet, at most 1472 payload bytes per packet
        const double packets = std::ceil(bytes / 1472.0);
        return 8.0 * (bytes + packets * (28 + 38));  // IP+UDP headers, Ethernet framing and gap
    }
    return -1.0;
}

int main()
{
    const std::map<std::string, double> capacity{
        {"can", 500000.0}, {"uart", 115200.0}, {"i2c", 400000.0}, {"eth", 100000000.0}};
    std::map<std::string, double> used;
    std::string bus, name;
    int bytes = 0;
    double rate = 0.0;
    std::printf("%-5s %-14s %8s %8s %13s %8s\n", "bus", "stream", "bytes", "rate_Hz", "bits/s",
                "% of bus");
    while (std::cin >> bus >> name >> bytes >> rate) {
        const double b = bitsPerMessage(bus, bytes);
        if (b < 0 || !capacity.contains(bus)) {
            std::printf("unknown bus %s\n", bus.c_str());
            return 1;
        }
        used[bus] += b * rate;
        std::printf("%-5s %-14s %8d %8.0f %13.0f %8.1f\n", bus.c_str(), name.c_str(), bytes, rate,
                    b * rate, 100.0 * b * rate / capacity.at(bus));
    }
    std::printf("\n");
    for (const auto& [b, bits] : used) {
        const double pct = 100.0 * bits / capacity.at(b);
        std::printf("%-5s total %13.0f of %11.0f bit/s = %5.1f %%  %s\n", b.c_str(), bits,
                    capacity.at(b), pct, pct > 70.0 ? "<- over the 70 % design limit" : "ok");
    }
    return 0;
}
