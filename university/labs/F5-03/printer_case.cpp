// printer_case.cpp - evidence generator for the forensic lab "The printer nobody can reach".
// Two laptops on the same wire print their settings; the ARP requests they broadcast when
// asked to print are saved to printer_case.pcap (simulated frames made by the university's
// own code, not a capture from a real office network).
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

struct Host
{
    std::string name;
    std::uint32_t address;
    int prefix;
    std::uint32_t gateway;
    std::uint8_t macLast;
};

constexpr std::uint32_t ip(unsigned a, unsigned b, unsigned c, unsigned d)
{
    return (a << 24) | (b << 16) | (c << 8) | d;
}

std::string dotted(std::uint32_t a)
{
    return std::to_string(a >> 24) + '.' + std::to_string((a >> 16) & 0xFF) + '.' +
           std::to_string((a >> 8) & 0xFF) + '.' + std::to_string(a & 0xFF);
}

std::uint32_t maskOf(int prefix) { return prefix == 0 ? 0u : ~std::uint32_t{0} << (32 - prefix); }

void put16(Bytes& b, unsigned v)
{
    b.push_back(static_cast<std::uint8_t>(v >> 8));
    b.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

void put32(Bytes& b, std::uint32_t v) { put16(b, v >> 16); put16(b, v & 0xFFFF); }

Bytes arpRequest(const Host& h, std::uint32_t wanted)
{
    const Bytes mac = {0x02, 0x00, 0x00, 0x41, 0x00, h.macLast};
    Bytes f(6, 0xff);                                       // broadcast
    f.insert(f.end(), mac.begin(), mac.end());
    put16(f, 0x0806); put16(f, 1); put16(f, 0x0800); f.push_back(6); f.push_back(4); put16(f, 1);
    f.insert(f.end(), mac.begin(), mac.end());
    put32(f, h.address);
    f.insert(f.end(), 6, 0x00);
    put32(f, wanted);
    return f;
}

int main()
{
    const std::vector<Host> hosts = {
        {"zainab-laptop", ip(192, 168, 41, 15), 24, ip(192, 168, 41, 1), 0x15},
        {"pedro-laptop", ip(192, 168, 41, 16), 16, ip(192, 168, 41, 1), 0x16},
    };
    const std::uint32_t printer = ip(192, 168, 40, 20);
    std::vector<Bytes> frames;
    for (const Host& h : hosts) {
        std::printf("%s %s/%d gateway %s\n", h.name.c_str(), dotted(h.address).c_str(), h.prefix,
                    dotted(h.gateway).c_str());
        const bool onLink = (printer & maskOf(h.prefix)) == (h.address & maskOf(h.prefix));
        frames.push_back(arpRequest(h, onLink ? printer : h.gateway));
    }
    Bytes reply = arpRequest(hosts[0], hosts[0].gateway);   // the router answers Zainab:
    for (int i = 0; i < 6; ++i) { reply[i] = reply[6 + i]; reply[32 + i] = reply[22 + i]; }
    reply[11] = 0x01; reply[27] = 0x01;                     // router MAC ends in 01
    reply[21] = 2;                                          // operation: reply
    for (int i = 0; i < 4; ++i) { std::swap(reply[28 + i], reply[38 + i]); }
    frames.insert(frames.begin() + 1, reply);
    Bytes file;
    auto le32 = [&file](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) { file.push_back(static_cast<std::uint8_t>(v >> (8 * i))); }
    };
    le32(0xa1b2c3d4); le32(0x00040002); le32(0); le32(0); le32(65535); le32(1);
    std::uint32_t second = 0;
    for (const Bytes& f : frames) {                         // Pedro's request is repeated below
        for (int repeat = 0; repeat < (f[31] == 16 ? 3 : 1); ++repeat) {
            const bool isReply = f[21] == 2;          // a reply comes 300 microseconds later
            le32(isReply ? second - 1 : second++); le32(isReply ? 300 : 0);
            const auto n = static_cast<std::uint32_t>(f.size());
            le32(n); le32(n);
            file.insert(file.end(), f.begin(), f.end());
        }
    }
    std::ofstream out("printer_case.pcap", std::ios::binary);
    const auto size = static_cast<std::streamsize>(file.size());
    out.write(reinterpret_cast<const char*>(file.data()), size);
    std::printf("saved printer_case.pcap (the ARP frames of the two print jobs)\n");
    return 0;
}
