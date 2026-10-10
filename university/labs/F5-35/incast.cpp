// incast.cpp - DS401 F5-35, forensic evidence generator ("one sender is always slow").
// A scripted replay, not a network simulation: three senders write 6-packet RDMA WRITE
// messages to one receiver at the same time (incast). The switch configuration below makes
// one traffic class lossless; packets of the lossy class are dropped where the script says
// the shared buffer was full. The receiver answers with ACKs, or with a NAK (PSN sequence
// error) when a PSN is missing; the sender then resends from the missing PSN (go-back-N).
// Output: incast.pcap (frames seen at the mirror of the receiver's switch port) and,
// on stdout, the configuration report the operators collected.
#include "packet.hpp"

#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace {

struct Sender
{
    std::string name;
    uint32_t ip;
    Mac mac;
    uint8_t dscp;
    uint32_t destQp;
    uint32_t firstPsn;
};

constexpr uint32_t kReceiverIp = ip4(10, 0, 0, 2);
constexpr Mac kReceiverMac{0x02};
constexpr uint8_t kLosslessDscp = 26;
constexpr int kPackets = 6;
constexpr size_t kPathMtu = 1024;

Bytes writePacket(const Sender& s, int index)
{
    const uint8_t opcode = index == 0 ? 0x06 : (index == kPackets - 1 ? 0x08 : 0x07);
    Bytes t = bth(opcode, s.destQp, s.firstPsn + static_cast<uint32_t>(index), index == kPackets - 1);
    if (index == 0) {
        append(t, reth(0x00007f0000100000ULL, 0x00002000, static_cast<uint32_t>(kPathMtu * kPackets)));
    }
    append(t, Bytes(kPathMtu, static_cast<uint8_t>('a' + index)));
    append(t, icrcPlaceholder());
    return roceV2(kReceiverMac, s.mac, s.ip, kReceiverIp, s.dscp, t);
}

Bytes ackPacket(const Sender& s, uint8_t syndrome, uint32_t psn)
{
    Bytes t = bth(0x11, s.destQp + 0x100, psn, false);
    append(t, aeth(syndrome, 1));
    append(t, icrcPlaceholder());
    return roceV2(s.mac, kReceiverMac, kReceiverIp, s.ip, kLosslessDscp, t);
}

}  // namespace

int main()
{
    const std::vector<Sender> senders = {
        {"node-a", ip4(10, 0, 0, 1), Mac{0x01}, 26, 0x000011, 100},
        {"node-c", ip4(10, 0, 0, 3), Mac{0x03}, 26, 0x000012, 200},
        {"node-d", ip4(10, 0, 0, 4), Mac{0x04}, 0, 0x000013, 300},
    };
    std::printf("=== configuration report collected by the operators ===\n");
    std::printf("switch port to node-b (10.0.0.2): traffic class 3 lossless (PFC on), DSCP 26 -> class 3;"
                " all other DSCP -> class 0 (lossy, PFC off); ECN marking off\n");
    for (const Sender& s : senders) {
        std::printf("%s %u.%u.%u.%u: RoCE v2, path MTU %zu, QP traffic class (DSCP) %u\n", s.name.c_str(),
                    s.ip >> 24, (s.ip >> 16) & 255, (s.ip >> 8) & 255, s.ip & 255, kPathMtu, s.dscp);
    }
    std::printf("node-b 10.0.0.2: receiver; capture = mirror of node-b's switch port, both directions\n");

    PcapWriter pcap("incast.pcap");
    uint32_t t = 0;
    // The script: in the lossy class, the 3rd and 4th packets of the burst find the buffer full.
    const std::set<int> droppedInLossyClass = {2, 3};
    std::vector<int> nextExpected(senders.size(), 0);
    std::vector<bool> nakSent(senders.size(), false);
    for (int i = 0; i < kPackets; ++i) {
        for (size_t k = 0; k < senders.size(); ++k) {
            const Sender& s = senders[k];
            const bool lossless = s.dscp == kLosslessDscp;
            if (!lossless && droppedInLossyClass.count(i) != 0) {
                continue;  // dropped before the mirror port: not in the capture
            }
            pcap.frame(t += 3, writePacket(s, i));
            if (i == nextExpected[k]) {
                ++nextExpected[k];
            } else if (!nakSent[k]) {
                nakSent[k] = true;  // out of sequence: one NAK, later packets discarded
                pcap.frame(t += 2, ackPacket(s, 0x60, s.firstPsn + static_cast<uint32_t>(nextExpected[k])));
            }
            if (i == kPackets - 1 && nextExpected[k] == kPackets) {
                pcap.frame(t += 2, ackPacket(s, 0x00, s.firstPsn + kPackets - 1));
            }
        }
    }
    // node-d's NIC goes back to the PSN in the NAK and resends everything from there.
    const size_t d = 2;
    for (int i = nextExpected[d]; i < kPackets; ++i) {
        pcap.frame(t += 3, writePacket(senders[d], i));
    }
    pcap.frame(t += 2, ackPacket(senders[d], 0x00, senders[d].firstPsn + kPackets - 1));
    std::printf("capture written: incast.pcap\n");
    return 0;
}
