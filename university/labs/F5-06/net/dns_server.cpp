// dns_server.cpp - a tiny authoritative DNS server for the lab's made-up zone "lab.example".
// It answers type A (IPv4 address) questions over UDP from a fixed table, says
// "no such name" (NXDOMAIN) for names it does not know, and exits after 2 s of silence.
// Usage: dns_server <port>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

// The zone, as the network administrator typed it.
const std::map<std::string, std::string> zone = {
    {"web.lab.example", "192.0.2.80"},
    {"fileserver.lab.example", "192.0.2.21"},
    {"pritner.lab.example", "192.0.2.20"},
};

std::uint16_t get16(const Bytes& m, std::size_t at)
{
    return static_cast<std::uint16_t>((m[at] << 8) | m[at + 1]);
}

void put16(Bytes& m, unsigned v)
{
    m.push_back(static_cast<std::uint8_t>(v >> 8));
    m.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

// A name is a list of labels, each a length byte then the characters, ending with 0.
// "web.lab.example" is 3 w e b 3 l a b 7 e x a m p l e 0.
std::optional<std::string> readName(const Bytes& m, std::size_t& at)
{
    std::string name;
    while (at < m.size() && m[at] != 0) {
        const std::size_t len = m[at++];
        if (len > 63 || at + len > m.size()) { return std::nullopt; }   // no compression here
        if (!name.empty()) { name += '.'; }
        for (std::size_t i = 0; i < len; ++i) {
            name += static_cast<char>(std::tolower(m[at + i]));
        }
        at += len;
    }
    ++at;                                                   // skip the final 0
    return name;
}

Bytes answer(const Bytes& q, std::size_t questionEnd, const std::string& name, unsigned qtype)
{
    const auto found = zone.find(name);
    Bytes r(q.begin(), q.begin() + static_cast<std::ptrdiff_t>(questionEnd));   // copy id+question
    const bool haveA = found != zone.end() && qtype == 1;
    const unsigned rcode = found == zone.end() ? 3 : 0;     // 3 = NXDOMAIN, "no such name"
    r[2] = static_cast<std::uint8_t>(0x84 | (q[2] & 0x01)); // QR=1 response, AA=1, copy RD
    r[3] = static_cast<std::uint8_t>(rcode);
    r[6] = 0; r[7] = haveA ? 1 : 0;                         // ANCOUNT
    r[8] = 0; r[9] = 0; r[10] = 0; r[11] = 0;               // NSCOUNT, ARCOUNT
    if (haveA) {
        put16(r, 0xC00C);              // name: "same as at byte 12" (compression pointer)
        put16(r, 1);                   // TYPE A
        put16(r, 1);                   // CLASS IN
        put16(r, 0); put16(r, 300);    // TTL: 300 seconds, a 32-bit number
        put16(r, 4);                   // RDLENGTH: an IPv4 address is 4 bytes
        in_addr a{};
        inet_pton(AF_INET, found->second.c_str(), &a);
        const auto* b = reinterpret_cast<const std::uint8_t*>(&a.s_addr);
        r.insert(r.end(), b, b + 4);   // already in network byte order
    }
    return r;
}

int main(int argc, char** argv)
{
    if (argc < 2) { std::fprintf(stderr, "usage: %s <port>\n", argv[0]); return 2; }
    const int s = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in me{};
    me.sin_family = AF_INET;
    me.sin_port = htons(static_cast<std::uint16_t>(std::stoi(argv[1])));
    me.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    timeval idle{2, 0};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &idle, sizeof idle);
    if (bind(s, reinterpret_cast<sockaddr*>(&me), sizeof me) != 0) {
        std::printf("bind failed: %s\n", std::strerror(errno));
        close(s);
        return 1;
    }
    std::printf("dns_server: authoritative for lab.example, %zu names\n", zone.size());
    std::fflush(stdout);
    for (;;) {
        Bytes q(512);
        sockaddr_in from{};
        socklen_t fromLen = sizeof from;
        const ssize_t n = recvfrom(s, q.data(), q.size(), 0,
                                   reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (n < 0) { break; }                               // 2 s of silence: stop
        q.resize(static_cast<std::size_t>(n));
        if (q.size() < 12 || get16(q, 4) != 1) { continue; }   // expect exactly one question
        std::size_t at = 12;
        const auto name = readName(q, at);
        if (!name || at + 4 > q.size()) { continue; }
        const unsigned qtype = get16(q, at);
        const Bytes r = answer(q, at + 4, *name, qtype);
        sendto(s, r.data(), r.size(), 0, reinterpret_cast<sockaddr*>(&from), fromLen);
        std::printf("query id=0x%04x %s type %u -> %s\n", get16(q, 0), name->c_str(), qtype,
                    r[3] == 3 ? "NXDOMAIN" : (r[7] == 1 ? zone.at(*name).c_str() : "no A record"));
        std::fflush(stdout);
    }
    close(s);
    return 0;
}
