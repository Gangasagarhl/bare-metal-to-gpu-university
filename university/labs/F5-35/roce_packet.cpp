// roce_packet.cpp - DS401 F5-35, Listing 1.
// Builds three frames byte by byte and writes them to roce.pcap for tshark to decode:
//   1. RoCE v2: RC RDMA WRITE Only (BTH + RETH + 32 payload bytes + ICRC placeholder)
//   2. RoCE v2: RC Acknowledge (BTH + AETH) going back
//   3. RoCE v1: RC SEND Only carried with a Global Route Header directly in Ethernet
// Then prints the header sizes and the payload efficiency for each InfiniBand path MTU.
#include "packet.hpp"

#include <cstdio>
#include <string>

namespace {

void hexdump(const char* title, const Bytes& f)
{
    std::printf("%s (%zu bytes)\n", title, f.size());
    for (size_t i = 0; i < f.size(); ++i) {
        std::printf("%s%02x", i % 16 == 0 ? (i ? "\n  " : "  ") : " ", f[i]);
    }
    std::printf("\n");
}

}  // namespace

int main()
{
    const Mac nodeA{0x01};
    const Mac nodeB{0x02};
    const uint32_t ipA = ip4(10, 0, 0, 1);
    const uint32_t ipB = ip4(10, 0, 0, 2);
    const std::string text = "DS401: written by an RDMA WRITE.";  // 32 bytes
    const Bytes payload(text.begin(), text.end());

    // 1. RDMA WRITE Only: opcode 0x0a in the RC group (0x00).
    Bytes write = bth(0x0a, 0x000011, 5, true);
    append(write, reth(0x00007f0000001000ULL, 0x00001234, static_cast<uint32_t>(payload.size())));
    append(write, payload);
    append(write, icrcPlaceholder());
    const Bytes f1 = roceV2(nodeB, nodeA, ipA, ipB, 26, write);

    // 2. Acknowledge: opcode 0x11, AETH syndrome 0x00 (positive ACK), MSN 1.
    Bytes ack = bth(0x11, 0x000022, 5, false);
    append(ack, aeth(0x00, 1));
    append(ack, icrcPlaceholder());
    const Bytes f2 = roceV2(nodeA, nodeB, ipB, ipA, 26, ack);

    // 3. RoCE v1: EtherType 0x8915, then a 40-byte Global Route Header (GRH), then BTH.
    Bytes v1 = ethernet(nodeB, nodeA, 0x8915);
    Bytes grh;
    put32(grh, 6u << 28);                 // IP version 6 field, traffic class 0, flow label 0
    const Bytes body(16, 0x5a);
    put16(grh, static_cast<uint32_t>(12 + body.size() + 4));  // payload length after the GRH
    put8(grh, 0x1b);                      // next header: the IBA transport (BTH)
    put8(grh, 64);                        // hop limit
    for (const uint32_t last : {1u, 2u}) {  // source and destination GID: fe80::1, fe80::2
        put16(grh, 0xfe80);
        for (int i = 0; i < 7; ++i) {
            put16(grh, i == 6 ? last : 0);
        }
    }
    append(v1, grh);
    append(v1, bth(0x04, 0x000011, 6, true));  // SEND Only
    append(v1, body);
    append(v1, icrcPlaceholder());

    PcapWriter pcap("roce.pcap");
    pcap.frame(0, f1);
    pcap.frame(5, f2);
    pcap.frame(10, v1);

    hexdump("frame 1: RoCE v2 RDMA WRITE Only", f1);
    hexdump("frame 2: RoCE v2 Acknowledge", f2);
    hexdump("frame 3: RoCE v1 SEND Only", v1);

    // Header bytes as captured (no Ethernet preamble, inter-frame gap or frame check sequence).
    const int eth = 14, ip = 20, udpLen = 8, bthLen = 12, rethLen = 16, icrc = 4;
    const int perPacket = eth + ip + udpLen + bthLen + icrc;
    std::printf("\nRoCE v2 header bytes per packet: Ethernet %d + IPv4 %d + UDP %d + BTH %d + ICRC %d = %d"
                " (+ RETH %d on the first packet of an RDMA WRITE)\n",
                eth, ip, udpLen, bthLen, icrc, perPacket, rethLen);
    std::printf("%-10s %-14s %-22s %-22s %s\n", "path MTU", "payload share", "IP packet (1st/other)",
                "fits Ethernet MTU 1500", "fits 9000");
    for (const int mtu : {256, 512, 1024, 2048, 4096}) {
        const int ipFirst = ip + udpLen + bthLen + rethLen + mtu + icrc;
        const int ipOther = ip + udpLen + bthLen + mtu + icrc;
        const double share = 100.0 * mtu / (mtu + perPacket);
        std::printf("%-10d %6.1f %%       %5d / %-5d          %-22s %s\n", mtu, share, ipFirst, ipOther,
                    ipFirst <= 1500 ? "yes" : "no", ipFirst <= 9000 ? "yes" : "no");
    }
    return 0;
}
