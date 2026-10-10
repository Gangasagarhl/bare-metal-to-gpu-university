// netcheck.cpp - DR302 F4-10: the header offsets and constants that net.cc and tcp.cc use,
// compared with the C library's own definitions (<net/ethernet.h>, <netinet/*.h>) in the
// build container (tier 4: what glibc defines). Also checks the Internet checksum code.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

namespace
{
int g_bad = 0;

void check(const char* name, unsigned ours, unsigned glibc)
{
    const bool ok = ours == glibc;
    std::printf("%-30s ours %5u  glibc %5u  %s\n", name, ours, glibc, ok ? "ok" : "MISMATCH");
    if (!ok) {
        ++g_bad;
    }
}

// The same algorithm as net::checksum (RFC 1071), for the self-test below.
std::uint16_t checksum(const std::uint8_t* p, std::size_t len)
{
    std::uint32_t s = 0;
    for (std::size_t i = 0; i + 1 < len; i += 2) {
        s += static_cast<std::uint32_t>(p[i] << 8 | p[i + 1]);
    }
    if (len & 1) {
        s += static_cast<std::uint32_t>(p[len - 1] << 8);
    }
    while (s >> 16) {
        s = (s & 0xFFFF) + (s >> 16);
    }
    return static_cast<std::uint16_t>(~s);
}
}  // namespace

int main()
{
    // Offsets used by net.cc / tcp.cc (in bytes from the start of each header).
    check("Ethernet type", 12, offsetof(struct ether_header, ether_type));
    check("ETHERTYPE_IP", 0x0800, ETHERTYPE_IP);
    check("ETHERTYPE_ARP", 0x0806, ETHERTYPE_ARP);
    check("ARP sender MAC", 8, offsetof(struct ether_arp, arp_sha));
    check("ARP sender IP", 14, offsetof(struct ether_arp, arp_spa));
    check("ARP target MAC", 18, offsetof(struct ether_arp, arp_tha));
    check("ARP target IP", 24, offsetof(struct ether_arp, arp_tpa));
    check("IPv4 total length", 2, offsetof(struct iphdr, tot_len));
    check("IPv4 identification", 4, offsetof(struct iphdr, id));
    check("IPv4 fragment field", 6, offsetof(struct iphdr, frag_off));
    check("IPv4 TTL", 8, offsetof(struct iphdr, ttl));
    check("IPv4 protocol", 9, offsetof(struct iphdr, protocol));
    check("IPv4 checksum", 10, offsetof(struct iphdr, check));
    check("IPv4 source", 12, offsetof(struct iphdr, saddr));
    check("IPv4 destination", 16, offsetof(struct iphdr, daddr));
    check("IPPROTO_ICMP", 1, IPPROTO_ICMP);
    check("IPPROTO_TCP", 6, IPPROTO_TCP);
    check("IPPROTO_UDP", 17, IPPROTO_UDP);
    check("ICMP echo request type", 8, ICMP_ECHO);
    check("ICMP echo reply type", 0, ICMP_ECHOREPLY);
    check("UDP length", 4, offsetof(struct udphdr, uh_ulen));
    check("UDP checksum", 6, offsetof(struct udphdr, uh_sum));
    check("TCP sequence number", 4, offsetof(struct tcphdr, th_seq));
    check("TCP acknowledgment number", 8, offsetof(struct tcphdr, th_ack));
    check("TCP flags byte", 13, offsetof(struct tcphdr, th_flags));
    check("TCP window", 14, offsetof(struct tcphdr, th_win));
    check("TCP checksum", 16, offsetof(struct tcphdr, th_sum));
    check("TH_FIN", 0x01, TH_FIN);
    check("TH_SYN", 0x02, TH_SYN);
    check("TH_RST", 0x04, TH_RST);
    check("TH_PUSH", 0x08, TH_PUSH);
    check("TH_ACK", 0x10, TH_ACK);
    check("TCPOPT_MAXSEG", 2, TCPOPT_MAXSEG);
    check("sizeof IPv4 header", 20, sizeof(struct iphdr));
    check("sizeof TCP header", 20, sizeof(struct tcphdr));

    // Checksum self-test: a header with its checksum filled in must sum to zero.
    std::uint8_t h[20] = {0x45, 0, 0, 84, 0, 1, 0x40, 0, 64, 1, 0, 0, 10, 0, 2, 15, 10, 0, 2, 2};
    const std::uint16_t c = checksum(h, 20);
    h[10] = static_cast<std::uint8_t>(c >> 8);
    h[11] = static_cast<std::uint8_t>(c & 0xFF);
    std::printf("checksum of the sample IPv4 header: 0x%04x; re-check over the filled header: 0x%04x %s\n", c,
                checksum(h, 20), checksum(h, 20) == 0 ? "ok" : "MISMATCH");
    if (checksum(h, 20) != 0) {
        ++g_bad;
    }
    std::printf("%s: %d mismatches\n", g_bad == 0 ? "PASS" : "FAIL", g_bad);
    return g_bad == 0 ? 0 : 1;
}
