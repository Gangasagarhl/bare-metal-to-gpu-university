// frame.cpp - F1-51 Listing 1: build one Ethernet frame byte by byte, as a MAC would
// send it: destination address, source address, EtherType, payload (an ARP request),
// padding up to the minimum size, then the 4-byte frame check sequence (CRC-32).
// It writes the frame to frame.pcap so that tshark (Wireshark's command-line tool) can
// decode it and check our CRC independently (run.sh does that).
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

// CRC-32 as used for the Ethernet frame check sequence: reflected polynomial 0xEDB88320,
// initial value 0xFFFFFFFF, final inversion; computed bit by bit for clarity.
static std::uint32_t crc32(const Bytes& data)
{
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::uint8_t byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        }
    }
    return ~crc;
}

static void put16(Bytes& b, std::uint16_t v)
{
    b.push_back(static_cast<std::uint8_t>(v >> 8));    // network byte order: high byte first
    b.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

static void put32le(std::ofstream& f, std::uint32_t v)
{
    const std::array<char, 4> b{static_cast<char>(v), static_cast<char>(v >> 8),
                                static_cast<char>(v >> 16), static_cast<char>(v >> 24)};
    f.write(b.data(), 4);
}

int main()
{
    const Bytes broadcast{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const Bytes myMac{0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

    Bytes frame;
    frame.insert(frame.end(), broadcast.begin(), broadcast.end());   // destination
    frame.insert(frame.end(), myMac.begin(), myMac.end());           // source
    put16(frame, 0x0806);                                            // EtherType: ARP

    // ARP request "who has 10.0.2.2? tell 10.0.2.15"
    put16(frame, 1);                 // hardware type: Ethernet
    put16(frame, 0x0800);            // protocol type: IPv4
    frame.push_back(6);              // hardware address length
    frame.push_back(4);              // protocol address length
    put16(frame, 1);                 // operation: request
    frame.insert(frame.end(), myMac.begin(), myMac.end());
    for (std::uint8_t b : {10, 0, 2, 15}) { frame.push_back(b); }
    for (int i = 0; i < 6; ++i) { frame.push_back(0); }              // target MAC unknown
    for (std::uint8_t b : {10, 0, 2, 2}) { frame.push_back(b); }

    const std::size_t unpadded = frame.size();
    while (frame.size() < 60) {      // pad so that frame + FCS reaches the minimum size
        frame.push_back(0);
    }
    const std::uint32_t fcs = crc32(frame);
    for (int i = 0; i < 4; ++i) {    // the FCS goes on the wire least significant byte first
        frame.push_back(static_cast<std::uint8_t>(fcs >> (8 * i)));
    }

    std::printf("header + ARP payload: %zu bytes, padded to 60, plus FCS = %zu bytes\n",
                unpadded, frame.size());
    std::printf("CRC-32 over the first 60 bytes: 0x%08X\n", fcs);
    std::printf("CRC-32 of the ASCII text \"123456789\": 0x%08X\n",
                crc32(Bytes{'1', '2', '3', '4', '5', '6', '7', '8', '9'}));
    for (std::size_t i = 0; i < frame.size(); ++i) {
        std::printf("%02x%s", frame[i], (i % 16 == 15 || i + 1 == frame.size()) ? "\n" : " ");
    }

    // Frame 2: the same frame with one bit flipped "on the wire" after the FCS was computed.
    Bytes damaged = frame;
    damaged[30] ^= 0x01;             // sender IP 10.0.2.15 becomes 10.0.3.15

    // A minimal pcap file: global header, then a record header before each frame.
    std::ofstream pcap("frame.pcap", std::ios::binary);
    put32le(pcap, 0xA1B2C3D4u);                          // magic number
    put32le(pcap, 2u | (4u << 16));                      // version 2.4
    put32le(pcap, 0);                                    // time zone offset
    put32le(pcap, 0);                                    // timestamp accuracy
    put32le(pcap, 65535);                                // snapshot length
    put32le(pcap, 1);                                    // link type 1: Ethernet
    for (const Bytes* f : {&frame, &damaged}) {
        put32le(pcap, 0);                                // seconds
        put32le(pcap, 0);                                // microseconds
        put32le(pcap, static_cast<std::uint32_t>(f->size()));   // bytes saved
        put32le(pcap, static_cast<std::uint32_t>(f->size()));   // bytes on the wire
        pcap.write(reinterpret_cast<const char*>(f->data()), static_cast<std::streamsize>(f->size()));
    }
    std::printf("CRC-32 of the damaged frame's first 60 bytes: 0x%08X (FCS carried: 0x%08X)\n",
                crc32(Bytes(damaged.begin(), damaged.begin() + 60)), fcs);
    return pcap ? 0 : 1;
}
