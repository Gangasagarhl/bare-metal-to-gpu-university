// net.h - DR302 F4-10: a small IPv4 stack for the lab kernel: Ethernet, ARP, IPv4, ICMP
// echo, UDP and a DHCP client. Header layouts follow RFC 826 (ARP), RFC 791 (IPv4),
// RFC 792 (ICMP), RFC 768 (UDP) and RFC 2131 (DHCP); checked against the C library's
// <netinet/*.h> and <net/ethernet.h> by netcheck.cpp, otherwise pending verification.
#pragma once
#include <stdint.h>

namespace net {
using Ip = uint32_t;                        // IPv4 address in host byte order
constexpr Ip ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) { return uint32_t{a} << 24 | b << 16 | c << 8 | d; }
inline uint16_t be16(const uint8_t* p) { return static_cast<uint16_t>(p[0] << 8 | p[1]); }
inline uint32_t be32(const uint8_t* p) { return uint32_t{p[0]} << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
inline void put16(uint8_t* p, uint16_t v) { p[0] = v >> 8; p[1] = v & 0xFF; }
inline void put32(uint8_t* p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
// Internet checksum (RFC 1071): one's complement of the one's complement sum of 16-bit words.
uint16_t checksum(const void* data, uint32_t len, uint32_t initial = 0);
uint32_t sum16(const void* data, uint32_t len, uint32_t acc);

constexpr uint16_t ETH_IPV4 = 0x0800, ETH_ARP = 0x0806;
constexpr uint8_t PROTO_ICMP = 1, PROTO_TCP = 6, PROTO_UDP = 17;

struct Config { uint8_t mac[6]; Ip addr, mask, gateway, dns, server; uint32_t lease_s; };
extern Config cfg;

struct Stats {
    uint32_t rx_frames, tx_frames, rx_dropped_loss, tx_dropped_loss, rx_bad_checksum, arp_replies;
};
extern Stats stats;

bool init();                                // finds and starts the e1000
void poll();                                // receive everything waiting, run TCP timers
bool dhcp(uint32_t timeout_ms);             // DISCOVER, OFFER, REQUEST, ACK
bool resolve(Ip addr, uint8_t mac[6], uint32_t timeout_ms);
int ping(Ip addr, uint16_t seq, uint32_t timeout_ms);     // round trip in ms, or -1
// Sends an IPv4 datagram (no fragmentation; payload at most 1480 bytes).
bool ip_send(Ip dst, uint8_t proto, const uint8_t* payload, uint16_t len);
// Induced loss for the lab: drop this percentage of TCP segments (deterministic hash).
void set_loss(uint32_t tx_percent, uint32_t rx_percent, uint32_t seed);
bool lose(uint32_t key, uint32_t percent);  // the hash decision itself
uint32_t loss_tx();
uint32_t loss_seed();
void print_ip(const char* label, Ip a);
}
