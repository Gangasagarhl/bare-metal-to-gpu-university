// arp_request.cpp - build the ARP request "who has 192.168.41.1? tell 192.168.41.15"
// inside an Ethernet broadcast frame, print it, and save it as arp.pcap for tshark.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

void put16(Bytes& b, unsigned v)
{
    b.push_back(static_cast<std::uint8_t>(v >> 8));
    b.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

void putAll(Bytes& b, const Bytes& more) { b.insert(b.end(), more.begin(), more.end()); }

Bytes arpRequest(const Bytes& myMac, const Bytes& myIp, const Bytes& wantedIp)
{
    Bytes f;
    putAll(f, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff});   // Ethernet destination: broadcast
    putAll(f, myMac);                                   // Ethernet source: me
    put16(f, 0x0806);                                   // EtherType: ARP
    put16(f, 1);                                        // hardware type: Ethernet
    put16(f, 0x0800);                                   // protocol type: IPv4
    f.push_back(6);                                     // hardware address length
    f.push_back(4);                                     // protocol address length
    put16(f, 1);                                        // operation: request
    putAll(f, myMac);                                   // sender MAC
    putAll(f, myIp);                                    // sender IP
    putAll(f, {0, 0, 0, 0, 0, 0});                      // target MAC: unknown, that is the question
    putAll(f, wantedIp);                                // target IP
    return f;
}

void savePcap(const char* path, const Bytes& frame)    // classic pcap, one record
{
    Bytes file;
    auto le32 = [&file](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) { file.push_back(static_cast<std::uint8_t>(v >> (8 * i))); }
    };
    le32(0xa1b2c3d4); le32(0x00040002); le32(0); le32(0); le32(65535); le32(1);
    le32(0); le32(0);
    le32(static_cast<std::uint32_t>(frame.size()));
    le32(static_cast<std::uint32_t>(frame.size()));
    putAll(file, frame);
    std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(file.data()),
                                                 static_cast<std::streamsize>(file.size()));
}

int main()
{
    const Bytes frame = arpRequest({0x02, 0x00, 0x00, 0x41, 0x00, 0x15},   // laptop's MAC
                                   {192, 168, 41, 15},                      // laptop's IP
                                   {192, 168, 41, 1});                      // the router's IP
    for (std::size_t i = 0; i < frame.size(); ++i) {
        std::printf("%02x%s", frame[i], (i % 14 == 13 || i + 1 == frame.size()) ? "\n" : " ");
    }
    std::printf("%zu bytes; saved arp.pcap\n", frame.size());
    savePcap("arp.pcap", frame);
    return 0;
}
