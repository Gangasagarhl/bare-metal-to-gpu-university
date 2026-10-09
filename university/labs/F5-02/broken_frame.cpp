// broken_frame.cpp - evidence for the forensic lab "The packets nobody accepts".
// Lin's version of encapsulate.cpp. It builds the same frame and saves it as broken.pcap.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

void put8(Bytes& b, unsigned v) { b.push_back(static_cast<std::uint8_t>(v & 0xFF)); }
void put16(Bytes& b, unsigned v) { put8(b, v >> 8); put8(b, v); }          // network order
void put32(Bytes& b, std::uint32_t v) { put16(b, v >> 16); put16(b, v & 0xFFFF); }

// Internet checksum: ones' complement of the ones' complement sum of 16-bit words.
std::uint16_t inetChecksum(const Bytes& b, std::uint32_t sum = 0)
{
    for (std::size_t i = 0; i < b.size(); i += 2) {
        sum += static_cast<std::uint32_t>(b[i] << 8) | (i + 1 < b.size() ? b[i + 1] : 0);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<std::uint16_t>(~sum & 0xFFFF);
}

void set16(Bytes& b, std::size_t at, std::uint16_t v)
{
    b[at] = static_cast<std::uint8_t>(v >> 8);
    b[at + 1] = static_cast<std::uint8_t>(v & 0xFF);
}

constexpr std::uint32_t srcIp = (192u << 24) | (0u << 16) | (2u << 8) | 1u;   // 192.0.2.1
constexpr std::uint32_t dstIp = (192u << 24) | (0u << 16) | (2u << 8) | 2u;   // 192.0.2.2

Bytes udpLayer(const std::string& text)                    // layer 4: transport
{
    Bytes u;
    put16(u, 40000);                                       // source port
    put16(u, 5000);                                        // destination port
    put16(u, static_cast<unsigned>(8 + text.size()));      // length: header + data
    put16(u, 0);                                           // checksum, filled below
    u.insert(u.end(), text.begin(), text.end());
    Bytes pseudo;                                          // "pseudo-header" from layer 3
    put32(pseudo, srcIp);
    put32(pseudo, dstIp);
    put16(pseudo, 17);                                     // protocol number of UDP
    put16(pseudo, static_cast<unsigned>(u.size()));
    pseudo.insert(pseudo.end(), u.begin(), u.end());
    set16(u, 6, inetChecksum(pseudo));
    return u;
}

Bytes ipLayer(const Bytes& payload)                        // layer 3: internet
{
    Bytes h;
    put8(h, 0x45);                                         // version 4, header 5 x 4 bytes
    put8(h, 0);                                            // type of service
    put16(h, static_cast<unsigned>(20 + payload.size()));  // total length
    put16(h, 1);                                           // identification
    put16(h, 0x4000);                                      // flags: don't fragment
    put8(h, 64);                                           // time to live
    put8(h, 17);                                           // protocol carried: UDP
    put16(h, 0);                                           // header checksum, filled below
    put32(h, srcIp);
    put32(h, dstIp);
    h.insert(h.end(), payload.begin(), payload.end());
    set16(h, 10, inetChecksum(h));
    return h;
}

Bytes ethernetLayer(const Bytes& payload)                  // layer 2: link
{
    Bytes f = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02,         // destination MAC
               0x02, 0x00, 0x00, 0x00, 0x00, 0x01};        // source MAC
    put16(f, 0x0800);                                      // EtherType: IPv4 inside
    f.insert(f.end(), payload.begin(), payload.end());
    return f;
}

void writePcap(const char* path, const Bytes& frame)       // classic pcap, one record
{
    Bytes file;
    auto le32 = [&file](std::uint32_t v) {                 // pcap fields: little-endian here
        for (int i = 0; i < 4; ++i) { file.push_back(static_cast<std::uint8_t>(v >> (8 * i))); }
    };
    le32(0xa1b2c3d4); le32(0x00040002); le32(0); le32(0); le32(65535); le32(1);
    le32(0); le32(0);                                      // timestamp: seconds, microseconds
    le32(static_cast<std::uint32_t>(frame.size()));        // bytes saved
    le32(static_cast<std::uint32_t>(frame.size()));        // bytes on the wire
    file.insert(file.end(), frame.begin(), frame.end());
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(file.data()),
                                                 static_cast<std::streamsize>(file.size()));
}

int main()
{
    const std::string text = "Hello, Amara!";
    const Bytes udp = udpLayer(text);
    const Bytes ip = ipLayer(udp);
    const Bytes frame = ethernetLayer(ip);
    std::printf("application %zu, +UDP header = %zu, +IPv4 header = %zu, +Ethernet header = %zu\n",
                text.size(), udp.size(), ip.size(), frame.size());
    for (std::size_t i = 0; i < frame.size(); ++i) {
        std::printf("%02x%s", frame[i], (i % 16 == 15 || i + 1 == frame.size()) ? "\n" : " ");
    }
    writePcap("broken.pcap", frame);
    std::printf("saved broken.pcap\n");
    return 0;
}
