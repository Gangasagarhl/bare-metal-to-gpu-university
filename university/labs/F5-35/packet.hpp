// packet.hpp - DS401 F5-35: build RoCE frames byte by byte and write them to a pcap file.
// Field layouts follow the InfiniBand transport headers as the Wireshark dissector
// (tshark 4.2.2) decodes them in this lab's run; see the chapter's sources.
#pragma once
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using Bytes = std::vector<uint8_t>;

inline void put8(Bytes& b, uint32_t v) { b.push_back(static_cast<uint8_t>(v)); }
inline void put16(Bytes& b, uint32_t v) { put8(b, v >> 8); put8(b, v); }               // big-endian
inline void put24(Bytes& b, uint32_t v) { put8(b, v >> 16); put16(b, v); }
inline void put32(Bytes& b, uint32_t v) { put16(b, v >> 16); put16(b, v); }
inline void put64(Bytes& b, uint64_t v) { put32(b, static_cast<uint32_t>(v >> 32)); put32(b, static_cast<uint32_t>(v)); }
inline void append(Bytes& b, const Bytes& more) { b.insert(b.end(), more.begin(), more.end()); }

struct Mac
{
    uint8_t last;  // 02:00:00:00:00:<last>, a locally administered address for the lab
};

inline Bytes ethernet(Mac dst, Mac src, uint16_t etherType)
{
    Bytes b;
    for (const Mac m : {dst, src}) {
        for (const uint8_t x : {0x02, 0x00, 0x00, 0x00, 0x00}) {
            put8(b, x);
        }
        put8(b, m.last);
    }
    put16(b, etherType);
    return b;  // 14 bytes
}

inline Bytes ipv4(uint32_t src, uint32_t dst, uint8_t dscp, uint8_t ecn, size_t payloadLen)
{
    Bytes b;
    put8(b, 0x45);                                   // version 4, header length 5 words
    put8(b, static_cast<uint32_t>(dscp << 2 | ecn));  // DSCP (6 bits) and ECN (2 bits)
    put16(b, static_cast<uint32_t>(20 + payloadLen));
    put16(b, 0x0001);                                 // identification
    put16(b, 0x4000);                                 // don't fragment
    put8(b, 64);                                      // time to live
    put8(b, 17);                                      // protocol: UDP
    put16(b, 0);                                      // checksum, filled in below
    put32(b, src);
    put32(b, dst);
    uint32_t sum = 0;
    for (size_t i = 0; i < b.size(); i += 2) {
        sum += static_cast<uint32_t>(b[i] << 8 | b[i + 1]);
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += sum >> 16;
    const uint32_t c = ~sum & 0xffff;
    b[10] = static_cast<uint8_t>(c >> 8);
    b[11] = static_cast<uint8_t>(c);
    return b;  // 20 bytes
}

inline Bytes udp(uint16_t srcPort, uint16_t dstPort, size_t payloadLen)
{
    Bytes b;
    put16(b, srcPort);
    put16(b, dstPort);
    put16(b, static_cast<uint32_t>(8 + payloadLen));
    put16(b, 0);  // checksum 0: "not computed" for UDP over IPv4
    return b;     // 8 bytes
}

// Base Transport Header: 12 bytes.
inline Bytes bth(uint8_t opcode, uint32_t destQp, uint32_t psn, bool ackRequest, uint8_t padCount = 0)
{
    Bytes b;
    put8(b, opcode);
    put8(b, static_cast<uint32_t>((padCount & 3) << 4));  // SE=0, MigReq=0, PadCnt, TVer=0
    put16(b, 0xffff);                                     // partition key (default partition)
    put8(b, 0);                                           // reserved
    put24(b, destQp);
    put8(b, ackRequest ? 0x80 : 0x00);                    // A bit and 7 reserved bits
    put24(b, psn);
    return b;
}

// RDMA Extended Transport Header: 16 bytes (virtual address, remote key, DMA length).
inline Bytes reth(uint64_t va, uint32_t rkey, uint32_t len)
{
    Bytes b;
    put64(b, va);
    put32(b, rkey);
    put32(b, len);
    return b;
}

// ACK Extended Transport Header: 4 bytes (syndrome, message sequence number).
inline Bytes aeth(uint8_t syndrome, uint32_t msn)
{
    Bytes b;
    put8(b, syndrome);
    put24(b, msn);
    return b;
}

// Invariant CRC placeholder: 4 bytes. NOT computed in this lab (see the chapter).
inline Bytes icrcPlaceholder() { return Bytes(4, 0); }

// One RoCE v2 frame: Ethernet + IPv4 + UDP (destination port 4791) + transport bytes.
inline Bytes roceV2(Mac dstMac, Mac srcMac, uint32_t srcIp, uint32_t dstIp, uint8_t dscp,
                    const Bytes& transport)
{
    Bytes u = udp(49152, 4791, transport.size());
    Bytes ip = ipv4(srcIp, dstIp, dscp, 0b10, u.size() + transport.size());
    Bytes f = ethernet(dstMac, srcMac, 0x0800);
    append(f, ip);
    append(f, u);
    append(f, transport);
    return f;
}

class PcapWriter
{
public:
    explicit PcapWriter(const std::string& path) : out_(path, std::ios::binary)
    {
        if (!out_) {
            throw std::runtime_error("cannot create " + path);
        }
        le32(0xa1b2c3d4);  // magic: microsecond timestamps
        le16(2);
        le16(4);           // file format version 2.4
        le32(0);
        le32(0);           // time zone, accuracy
        le32(65535);       // snapshot length
        le32(1);           // link type 1: Ethernet
    }
    void frame(uint32_t usec, const Bytes& f)
    {
        le32(usec / 1000000);
        le32(usec % 1000000);
        le32(static_cast<uint32_t>(f.size()));
        le32(static_cast<uint32_t>(f.size()));
        out_.write(reinterpret_cast<const char*>(f.data()), static_cast<std::streamsize>(f.size()));
    }

private:
    void le16(uint32_t v)
    {
        const char b[2] = {static_cast<char>(v), static_cast<char>(v >> 8)};
        out_.write(b, 2);
    }
    void le32(uint32_t v)
    {
        le16(v);
        le16(v >> 16);
    }
    std::ofstream out_;
};

constexpr uint32_t ip4(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return a << 24 | b << 16 | c << 8 | d;
}
