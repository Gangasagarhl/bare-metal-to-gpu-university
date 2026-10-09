// net.cc - DR302 F4-10: Ethernet, ARP, IPv4, ICMP, UDP and DHCP over the e1000 driver.
#include "net.h"
#include "tcp.h"
#include "e1000.h"
#include "../F4-01/kbase.h"
#include "../F4-08/intr.h"

namespace net {
Config cfg;
Stats stats;
}

namespace {
using namespace net;
struct ArpEntry { Ip addr; uint8_t mac[6]; bool valid; };
ArpEntry g_arp[8];
uint8_t g_frame[1536];                         // receive buffer
uint16_t g_ip_id = 1;
uint32_t g_loss_tx = 0, g_loss_rx = 0, g_seed = 0;
// Waiters: the last ICMP echo reply and the last DHCP message seen.
volatile int g_echo_seq = -1;
uint8_t g_dhcp[600];
volatile uint16_t g_dhcp_len = 0;
const uint8_t kBroadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void eth_send(const uint8_t* dst, uint16_t type, const uint8_t* payload, uint16_t len)
{
    uint8_t f[1536];
    memcpy(f, dst, 6);
    memcpy(f + 6, cfg.mac, 6);
    put16(f + 12, type);
    memcpy(f + 14, payload, len);
    e1000::send(f, static_cast<uint16_t>(14 + len));
    ++stats.tx_frames;
}

void arp_send(uint16_t op, const uint8_t* tmac, Ip tip)
{
    uint8_t a[28];
    put16(a, 1);                                // hardware type: Ethernet
    put16(a + 2, ETH_IPV4);                     // protocol type: IPv4
    a[4] = 6; a[5] = 4;
    put16(a + 6, op);                           // 1 request, 2 reply
    memcpy(a + 8, cfg.mac, 6);
    put32(a + 14, cfg.addr);
    memcpy(a + 18, tmac, 6);
    put32(a + 24, tip);
    eth_send(op == 1 ? kBroadcast : tmac, ETH_ARP, a, 28);
}

void arp_learn(Ip addr, const uint8_t* mac)
{
    int free = -1;
    for (int i = 0; i < 8; ++i) {
        if (g_arp[i].valid && g_arp[i].addr == addr) { memcpy(g_arp[i].mac, mac, 6); return; }
        if (!g_arp[i].valid && free < 0) free = i;
    }
    if (free < 0) free = 0;
    g_arp[free].addr = addr;
    memcpy(g_arp[free].mac, mac, 6);
    g_arp[free].valid = true;
}

void arp_input(const uint8_t* a, uint16_t len)
{
    if (len < 28 || be16(a) != 1 || be16(a + 2) != ETH_IPV4) return;
    const uint16_t op = be16(a + 6);
    const Ip sip = be32(a + 14), tip = be32(a + 24);
    arp_learn(sip, a + 8);
    if (op == 2) ++stats.arp_replies;
    if (op == 1 && cfg.addr && tip == cfg.addr) arp_send(2, a + 8, sip);   // answer requests for us
}

void icmp_input(Ip src, const uint8_t* p, uint16_t len)
{
    if (len < 8 || checksum(p, len) != 0) { ++stats.rx_bad_checksum; return; }
    if (p[0] == 0) g_echo_seq = be16(p + 6);                    // echo reply
    if (p[0] == 8) {                                            // echo request: answer it
        uint8_t r[1480];
        memcpy(r, p, len);
        r[0] = 0;
        put16(r + 2, 0);
        put16(r + 2, checksum(r, len));
        ip_send(src, PROTO_ICMP, r, len);
    }
}

void udp_input(const uint8_t* p, uint16_t len)
{
    if (len < 8) return;
    if (be16(p + 2) == 68 && len - 8 <= static_cast<int>(sizeof g_dhcp)) {   // to the DHCP client port
        memcpy(g_dhcp, p + 8, len - 8);
        g_dhcp_len = static_cast<uint16_t>(len - 8);
    }
}

void ip_input(const uint8_t* p, uint16_t len)
{
    if (len < 20 || (p[0] >> 4) != 4) return;
    const uint16_t hl = (p[0] & 0xF) * 4, total = be16(p + 2);
    if (hl < 20 || total > len || total < hl) return;
    if (checksum(p, hl) != 0) { ++stats.rx_bad_checksum; return; }
    if (be16(p + 6) & 0x3FFF) return;                          // fragments: not supported
    const Ip src = be32(p + 12), dst = be32(p + 16);
    if (cfg.addr && dst != cfg.addr && dst != 0xFFFFFFFFu) return;
    const uint8_t* pl = p + hl;
    const uint16_t plen = static_cast<uint16_t>(total - hl);
    if (p[9] == PROTO_ICMP) icmp_input(src, pl, plen);
    else if (p[9] == PROTO_UDP) udp_input(pl, plen);
    else if (p[9] == PROTO_TCP) {
        // Induced receive loss: decided by the segment's sequence number and the IP id, so a
        // retransmission (new id) gets a fresh decision.
        if (g_loss_rx && plen >= 20 && lose(be32(pl + 4) ^ (uint32_t{be16(p + 4)} << 16) ^ 0x5A5A, g_loss_rx)) {
            ++stats.rx_dropped_loss;
            return;
        }
        tcp::input(src, dst, pl, plen);
    }
}

bool udp_send(Ip dst, uint16_t sport, uint16_t dport, const uint8_t* data, uint16_t len)
{
    uint8_t u[1480];
    put16(u, sport);
    put16(u + 2, dport);
    put16(u + 4, static_cast<uint16_t>(8 + len));
    put16(u + 6, 0);                                           // checksum optional for IPv4 UDP
    memcpy(u + 8, data, len);
    return ip_send(dst, PROTO_UDP, u, static_cast<uint16_t>(8 + len));
}

uint8_t* dhcp_option(uint8_t* o, uint8_t code, uint8_t len, const void* v)
{
    o[0] = code;
    o[1] = len;
    memcpy(o + 2, v, len);
    return o + 2 + len;
}

uint16_t dhcp_build(uint8_t* m, uint8_t type, uint32_t xid, Ip requested, Ip server)
{
    memset(m, 0, 300);
    m[0] = 1; m[1] = 1; m[2] = 6;                              // BOOTREQUEST, Ethernet, 6-byte MAC
    put32(m + 4, xid);
    put16(m + 10, 0x8000);                                     // "please broadcast the reply"
    memcpy(m + 28, cfg.mac, 6);
    put32(m + 236, 0x63825363);                                // magic cookie
    uint8_t* o = m + 240;
    o = dhcp_option(o, 53, 1, &type);                          // message type
    if (requested) { uint8_t b[4]; put32(b, requested); o = dhcp_option(o, 50, 4, b); }
    if (server) { uint8_t b[4]; put32(b, server); o = dhcp_option(o, 54, 4, b); }
    const uint8_t want[3] = {1, 3, 6};                         // mask, router, DNS
    o = dhcp_option(o, 55, 3, want);
    *o++ = 255;
    return static_cast<uint16_t>(o - m);
}

// Waits for a DHCP reply of 'type' with our xid; parses the options into cfg.
bool dhcp_wait(uint8_t type, uint32_t xid, Ip& yiaddr, uint32_t timeout_ms)
{
    const uint64_t end = clock::ms() + timeout_ms;
    while (clock::ms() < end) {
        poll();
        if (!g_dhcp_len) { intr::wait(); continue; }
        const uint8_t* m = g_dhcp;
        const uint16_t n = g_dhcp_len;
        g_dhcp_len = 0;
        if (n < 240 || m[0] != 2 || be32(m + 4) != xid || be32(m + 236) != 0x63825363) continue;
        uint8_t got = 0;
        Config c = cfg;
        for (uint16_t i = 240; i + 1 < n && m[i] != 255;) {
            if (m[i] == 0) { ++i; continue; }
            const uint8_t code = m[i], len = m[i + 1];
            const uint8_t* v = m + i + 2;
            if (code == 53) got = v[0];
            if (code == 1 && len == 4) c.mask = be32(v);
            if (code == 3 && len >= 4) c.gateway = be32(v);
            if (code == 6 && len >= 4) c.dns = be32(v);
            if (code == 54 && len == 4) c.server = be32(v);
            if (code == 51 && len == 4) c.lease_s = be32(v);
            i = static_cast<uint16_t>(i + 2 + len);
        }
        if (got != type) continue;
        yiaddr = be32(m + 16);
        c.addr = cfg.addr;
        cfg = c;
        return true;
    }
    return false;
}
}  // namespace

namespace net {
uint32_t sum16(const void* data, uint32_t len, uint32_t acc)
{
    const auto* p = static_cast<const uint8_t*>(data);
    for (uint32_t i = 0; i + 1 < len; i += 2) acc += uint32_t{p[i]} << 8 | p[i + 1];
    if (len & 1) acc += uint32_t{p[len - 1]} << 8;
    return acc;
}

uint16_t checksum(const void* data, uint32_t len, uint32_t initial)
{
    uint32_t s = sum16(data, len, initial);
    while (s >> 16) s = (s & 0xFFFF) + (s >> 16);
    return static_cast<uint16_t>(~s);
}

void print_ip(const char* label, Ip a)
{
    kprintf("%s%u.%u.%u.%u", label, a >> 24, (a >> 16) & 0xFF, (a >> 8) & 0xFF, a & 0xFF);
}

bool init()
{
    PciAddr a;
    if (!e1000::find(a) || !e1000::init(a)) return false;
    memcpy(cfg.mac, e1000::mac(), 6);
    return true;
}

void poll()
{
    for (int budget = 0; budget < 64; ++budget) {
        const uint16_t n = e1000::receive(g_frame, sizeof g_frame);
        if (n == 0) break;
        ++stats.rx_frames;
        if (n < 14) continue;
        const uint16_t type = be16(g_frame + 12);
        if (type == ETH_ARP) arp_input(g_frame + 14, static_cast<uint16_t>(n - 14));
        else if (type == ETH_IPV4) ip_input(g_frame + 14, static_cast<uint16_t>(n - 14));
    }
    tcp::timers();
}

bool resolve(Ip addr, uint8_t mac[6], uint32_t timeout_ms)
{
    const Ip hop = (addr & cfg.mask) == (cfg.addr & cfg.mask) ? addr : cfg.gateway;
    const uint64_t end = clock::ms() + timeout_ms;
    uint64_t next_try = 0;
    while (clock::ms() < end) {
        for (int i = 0; i < 8; ++i)
            if (g_arp[i].valid && g_arp[i].addr == hop) { memcpy(mac, g_arp[i].mac, 6); return true; }
        if (clock::ms() >= next_try) {
            const uint8_t zero[6] = {};
            arp_send(1, zero, hop);
            next_try = clock::ms() + 200;
        }
        poll();
        intr::wait();
    }
    return false;
}

bool ip_send(Ip dst, uint8_t proto, const uint8_t* payload, uint16_t len)
{
    if (len > 1480) return false;
    uint8_t mac[6];
    if (dst == 0xFFFFFFFFu) memcpy(mac, kBroadcast, 6);
    else if (!resolve(dst, mac, 1000)) return false;
    uint8_t h[1500];                                           // local: ip_send can nest via poll()
    h[0] = 0x45;                                               // version 4, 5-word header
    h[1] = 0;
    put16(h + 2, static_cast<uint16_t>(20 + len));
    put16(h + 4, g_ip_id++);
    put16(h + 6, 0x4000);                                      // don't fragment
    h[8] = 64;                                                 // time to live
    h[9] = proto;
    put16(h + 10, 0);
    put32(h + 12, cfg.addr);
    put32(h + 16, dst);
    put16(h + 10, checksum(h, 20));
    memcpy(h + 20, payload, len);
    eth_send(mac, ETH_IPV4, h, static_cast<uint16_t>(20 + len));
    return true;
}

bool dhcp(uint32_t timeout_ms)
{
    const uint32_t xid = 0x44523032;                           // "DR02"
    uint8_t m[300];
    Ip offered = 0;
    cfg.addr = 0;
    udp_send(0xFFFFFFFFu, 68, 67, m, dhcp_build(m, 1, xid, 0, 0));          // DISCOVER
    if (!dhcp_wait(2, xid, offered, timeout_ms)) return false;             // OFFER
    print_ip("dhcp: offer ", offered);
    print_ip(" from server ", cfg.server);
    kprintf("\n");
    udp_send(0xFFFFFFFFu, 68, 67, m, dhcp_build(m, 3, xid, offered, cfg.server));   // REQUEST
    Ip acked = 0;
    if (!dhcp_wait(5, xid, acked, timeout_ms)) return false;               // ACK
    cfg.addr = acked;
    print_ip("dhcp: bound to ", cfg.addr);
    print_ip(", mask ", cfg.mask);
    print_ip(", router ", cfg.gateway);
    print_ip(", DNS ", cfg.dns);
    kprintf(", lease %u s\n", cfg.lease_s);
    return true;
}

int ping(Ip addr, uint16_t seq, uint32_t timeout_ms)
{
    uint8_t e[64];
    e[0] = 8; e[1] = 0;                                        // echo request
    put16(e + 2, 0);
    put16(e + 4, 0x0302);                                      // identifier
    put16(e + 6, seq);
    for (int i = 8; i < 64; ++i) e[i] = static_cast<uint8_t>(i);
    put16(e + 2, checksum(e, 64));
    g_echo_seq = -1;
    const uint64_t start = clock::ms();
    if (!ip_send(addr, PROTO_ICMP, e, 64)) return -1;
    while (clock::ms() - start < timeout_ms) {
        poll();
        if (g_echo_seq == seq) return static_cast<int>(clock::ms() - start);
        intr::wait();
    }
    return -1;
}

void set_loss(uint32_t tx, uint32_t rx, uint32_t seed) { g_loss_tx = tx; g_loss_rx = rx; g_seed = seed; }
uint32_t loss_tx() { return g_loss_tx; }
uint32_t loss_seed() { return g_seed; }

bool lose(uint32_t key, uint32_t percent)
{
    uint32_t h = key ^ g_seed;                                 // integer hash (xorshift-multiply)
    h ^= h >> 16; h *= 0x7FEB352Du; h ^= h >> 15; h *= 0x846CA68Bu; h ^= h >> 16;
    return h % 100 < percent;
}
}  // namespace net
