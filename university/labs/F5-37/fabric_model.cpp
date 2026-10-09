// fabric_model.cpp - DS401 F5-37, Listing 2: the university's RoCE path model.
// Reads a configuration dump (stdin) and predicts the goodput of a long RDMA WRITE stream
// on each node pair, as a percentage of the link rate. A MODEL, not a measurement: its
// three mechanisms are (1) header bytes per packet, set by the path MTU; (2) drops in a
// lossy traffic class during incast, with a fixed per-packet probability; (3) go-back-N
// recovery: after a drop, everything already in flight is resent. A lossless class with
// priority flow control (PFC) pauses senders instead of dropping (pauses are counted).
// Usage: fabric_model < fabric_model.in            the pairs as configured
//        fabric_model whatif < fabric_model.in     also pair C-D with each setting fixed
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct PairConfig
{
    std::string name;
    int ethMtu = 0;
    int pathMtu = 0;
    int qpDscp = 0;
    int losslessDscp = 0;
    bool pfc = false;
    bool ecn = false;
    int incast = 1;
};

struct ModelParams
{
    double lossLossy = 0;
    double bdpBytes = 0;
    long packets = 0;
};

struct Result
{
    double goodputPct;
    long sent;
    long resent;
    long naks;
    long pauses;
};

constexpr int kHeaderBytes = 58;   // Ethernet 14 + IPv4 20 + UDP 8 + BTH 12 + ICRC 4 (F5-35)

class Lcg                          // deterministic pseudo-random numbers: same output every run
{
public:
    double next()
    {
        state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(state_ >> 11) / 9007199254740992.0;   // [0, 1)
    }

private:
    uint64_t state_ = 0x0D5401ULL;
};

Result simulate(const PairConfig& c, const ModelParams& m)
{
    const bool lossless = c.pfc && c.qpDscp == c.losslessDscp;
    const bool congested = c.incast > 1;
    const long window = static_cast<long>(std::ceil(m.bdpBytes / (c.pathMtu + kHeaderBytes)));
    Lcg rng;
    long delivered = 0, sent = 0, resent = 0, naks = 0, pauses = 0;
    while (delivered < m.packets) {
        ++sent;
        const bool hit = congested && rng.next() < m.lossLossy;
        if (hit && lossless) {
            ++pauses;          // PFC: the switch pauses the sender; nothing is lost
        } else if (hit) {
            ++naks;            // dropped; the next packet is out of sequence -> NAK
            sent += window - 1;   // go-back-N: the packets in flight behind it are discarded,
            resent += window;     // and all of them, with the lost one, are sent again
        }
        if (!hit || lossless) {
            ++delivered;
        }
    }
    // The link carries every sent packet with its headers; congestion also shares the link
    // among incast senders, so goodput is reported relative to this sender's fair share.
    const double useful = static_cast<double>(delivered) * c.pathMtu;
    const double carried = static_cast<double>(sent) * (c.pathMtu + kHeaderBytes);
    return Result{100.0 * useful / carried, sent, resent, naks, pauses};
}

void print(const PairConfig& c, const Result& r, const char* note)
{
    std::printf("%-6s path MTU %4d  PFC %-3s  QP DSCP %2d  ->  goodput %5.1f %% of fair share   "
                "sent %7ld  resent %7ld  NAKs %5ld  pauses %5ld  %s\n",
                c.name.c_str(), c.pathMtu, c.pfc ? "on" : "off", c.qpDscp, r.goodputPct, r.sent, r.resent,
                r.naks, r.pauses, note);
}

}  // namespace

int main(int argc, char** argv)
{
    std::vector<PairConfig> pairs;
    ModelParams m;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "pair") {
            PairConfig c;
            std::string pfc, ecn;
            in >> c.name >> c.ethMtu >> c.pathMtu >> c.qpDscp >> c.losslessDscp >> pfc >> ecn >> c.incast;
            c.pfc = pfc == "on";
            c.ecn = ecn == "on";
            pairs.push_back(c);
        } else if (kind == "model") {
            std::string key;
            while (in >> key) {
                if (key == "loss_lossy") in >> m.lossLossy;
                else if (key == "bdp_bytes") in >> m.bdpBytes;
                else if (key == "packets") in >> m.packets;
            }
        }
    }
    std::printf("RoCE path model (DS401 F5-37): loss in lossy class %.4f per packet during incast, "
                "%.0f bytes in flight, %ld packets delivered per run\n", m.lossLossy, m.bdpBytes, m.packets);
    for (const PairConfig& c : pairs) {
        print(c, simulate(c, m), "(as configured)");
    }
    if (argc > 1 && std::string(argv[1]) == "whatif") {
        for (const PairConfig& c : pairs) {
            if (c.name != "C-D") {
                continue;
            }
            PairConfig mtu = c;
            mtu.pathMtu = 4096;
            print(mtu, simulate(mtu, m), "(what if: Ethernet MTU raised so path MTU 4096)");
            PairConfig pfc = c;
            pfc.pfc = true;
            print(pfc, simulate(pfc, m), "(what if: PFC on for the lossless class)");
            PairConfig both = mtu;
            both.pfc = true;
            print(both, simulate(both, m), "(what if: both)");
        }
    }
    return 0;
}
