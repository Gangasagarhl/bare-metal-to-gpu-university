// dhcp_frames.cpp - build the four messages of a DHCP exchange (Discover, Offer, Request,
// Ack) as complete Ethernet frames and save them as dhcp.pcap, so that tshark can decode
// and check them. These are frames made by our own code, not a capture of a real network.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

void put8(Bytes& b, unsigned v) { b.push_back(static_cast<std::uint8_t>(v & 0xFF)); }
void put16(Bytes& b, unsigned v) { put8(b, v >> 8); put8(b, v); }
void put32(Bytes& b, std::uint32_t v) { put16(b, v >> 16); put16(b, v & 0xFFFF); }
void putAll(Bytes& b, const Bytes& more) { b.insert(b.end(), more.begin(), more.end()); }

constexpr std::uint32_t ip(unsigned a, unsigned b, unsigned c, unsigned d)
{
    return (a << 24) | (b << 16) | (c << 8) | d;
}

const Bytes clientMac = {0x02, 0x00, 0x00, 0x00, 0xaa, 0x01};
const Bytes serverMac = {0x02, 0x00, 0x00, 0x41, 0x00, 0x01};
const Bytes broadcastMac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
constexpr std::uint32_t serverIp = ip(192, 168, 41, 1);
constexpr std::uint32_t offeredIp = ip(192, 168, 41, 57);
constexpr std::uint32_t xid = 0x3903F326;            // the client's transaction number

// The DHCP message: a fixed BOOTP part, the "magic cookie", then options (code, length, data).
Bytes dhcp(unsigned op, unsigned type, std::uint32_t yiaddr, const Bytes& moreOptions)
{
    Bytes m;
    put8(m, op);                    // 1 = request from a client, 2 = reply from a server
    put8(m, 1); put8(m, 6); put8(m, 0);           // hardware type Ethernet, MAC length 6, hops
    put32(m, xid);
    put16(m, 0); put16(m, 0);                     // seconds, flags
    put32(m, 0);                                  // ciaddr: the client has no address yet
    put32(m, yiaddr);                             // yiaddr: "your" address, from the server
    put32(m, 0); put32(m, 0);                     // siaddr, giaddr
    putAll(m, clientMac);
    m.insert(m.end(), 10 + 64 + 128, 0);          // rest of chaddr, sname, file: unused
    put32(m, 0x63825363);                         // magic cookie: options follow
    put8(m, 53); put8(m, 1); put8(m, type);       // option 53: the DHCP message type
    putAll(m, moreOptions);
    put8(m, 255);                                 // end of options
    return m;
}

std::uint16_t checksum(const Bytes& b)
{
    std::uint32_t sum = 0;
    for (std::size_t i = 0; i + 1 < b.size(); i += 2) { sum += (b[i] << 8) | b[i + 1]; }
    while (sum >> 16) { sum = (sum & 0xFFFF) + (sum >> 16); }
    return static_cast<std::uint16_t>(~sum);
}

Bytes frame(const Bytes& dstMac, const Bytes& srcMac, std::uint32_t srcIp, std::uint32_t dstIp,
            unsigned srcPort, unsigned dstPort, const Bytes& payload)
{
    Bytes udp;
    put16(udp, srcPort); put16(udp, dstPort);
    put16(udp, static_cast<unsigned>(8 + payload.size())); put16(udp, 0);   // 0: no checksum
    putAll(udp, payload);
    Bytes iph;
    put8(iph, 0x45); put8(iph, 0); put16(iph, static_cast<unsigned>(20 + udp.size()));
    put16(iph, 0); put16(iph, 0); put8(iph, 64); put8(iph, 17); put16(iph, 0);
    put32(iph, srcIp); put32(iph, dstIp);
    const std::uint16_t c = checksum(iph);
    iph[10] = static_cast<std::uint8_t>(c >> 8);
    iph[11] = static_cast<std::uint8_t>(c & 0xFF);
    Bytes f = dstMac;
    putAll(f, srcMac);
    put16(f, 0x0800);
    putAll(f, iph);
    putAll(f, udp);
    return f;
}

int main()
{
    Bytes serverOptions;                                              // what the lease includes
    put8(serverOptions, 54); put8(serverOptions, 4); put32(serverOptions, serverIp);  // server id
    put8(serverOptions, 51); put8(serverOptions, 4); put32(serverOptions, 3600);      // lease (s)
    put8(serverOptions, 1); put8(serverOptions, 4); put32(serverOptions, ip(255, 255, 255, 0));
    put8(serverOptions, 3); put8(serverOptions, 4); put32(serverOptions, serverIp);   // router
    put8(serverOptions, 6); put8(serverOptions, 4); put32(serverOptions, serverIp);   // DNS
    Bytes wish;
    put8(wish, 55); put8(wish, 3); put8(wish, 1); put8(wish, 3); put8(wish, 6);       // please send
    Bytes request;
    put8(request, 50); put8(request, 4); put32(request, offeredIp);  // the address I accept
    put8(request, 54); put8(request, 4); put32(request, serverIp);   // from this server

    const std::uint32_t any = 0;
    const std::uint32_t all = ip(255, 255, 255, 255);
    const std::vector<Bytes> frames = {
        frame(broadcastMac, clientMac, any, all, 68, 67, dhcp(1, 1, 0, wish)),                // D
        frame(broadcastMac, serverMac, serverIp, all, 67, 68, dhcp(2, 2, offeredIp, serverOptions)),
        frame(broadcastMac, clientMac, any, all, 68, 67, dhcp(1, 3, 0, request)),             // R
        frame(broadcastMac, serverMac, serverIp, all, 67, 68, dhcp(2, 5, offeredIp, serverOptions)),
    };
    Bytes file;
    auto le32 = [&file](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) { file.push_back(static_cast<std::uint8_t>(v >> (8 * i))); }
    };
    le32(0xa1b2c3d4); le32(0x00040002); le32(0); le32(0); le32(65535); le32(1);
    std::uint32_t ms = 0;
    for (const Bytes& f : frames) {
        le32(0); le32(ms * 1000); ms += 2;        // 2 ms apart (made-up times)
        le32(static_cast<std::uint32_t>(f.size())); le32(static_cast<std::uint32_t>(f.size()));
        putAll(file, f);
        std::printf("frame of %zu bytes (DHCP message %zu bytes)\n", f.size(), f.size() - 42);
    }
    std::ofstream out("dhcp.pcap", std::ios::binary);
    const auto size = static_cast<std::streamsize>(file.size());
    out.write(reinterpret_cast<const char*>(file.data()), size);
    std::printf("saved dhcp.pcap\n");
    return 0;
}
