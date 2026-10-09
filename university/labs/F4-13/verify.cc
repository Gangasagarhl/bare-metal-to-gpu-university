// verify.cc - DR302 F4-13 host tool: the remote verifier. Reads the attestation report the
// lab kernel printed ("report: event ..." and "report: pcr ..." lines on stdin), then
//   1. for events whose data IS what was measured (EV_ACTION, EV_IPL), checks
//      digest == SHA-256(data);
//   2. replays every digest into software PCRs and compares them with the reported values.
// In a real attestation the PCR values arrive in a quote signed by the TPM; here they are
// the kernel's own PCR_Read results (no signing key exists in the mock TPM).
#include "sha256.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::vector<uint8_t> unhex(const std::string& h)
{
    std::vector<uint8_t> v;
    for (size_t i = 0; i + 1 < h.size(); i += 2) v.push_back(static_cast<uint8_t>(std::stoi(h.substr(i, 2), nullptr, 16)));
    return v;
}
std::string hex(const uint8_t* d, size_t n)
{
    static const char* x = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) { s += x[d[i] >> 4]; s += x[d[i] & 15]; }
    return s;
}
}  // namespace

int main()
{
    uint8_t soft[24][32] = {};
    std::vector<std::pair<unsigned, std::vector<uint8_t>>> reported;
    std::string line;
    int bad = 0, events = 0;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string tag, kind;
        in >> tag >> kind;
        if (tag != "report:") continue;
        if (kind == "event") {
            std::string w, type, dig, data;
            unsigned pcr;
            in >> w >> pcr >> w >> type >> w >> dig >> w >> data;
            const auto d = unhex(dig), payload = unhex(data);
            const unsigned t = static_cast<unsigned>(std::stoul(type, nullptr, 16));
            std::printf("event %2d: PCR %2u type 0x%-2x %4zu data bytes", ++events, pcr, t, payload.size());
            if (t == 0x5 || t == 0xD) {
                uint8_t h[32];
                sha::hash(payload.data(), payload.size(), h);
                const bool ok = std::memcmp(h, d.data(), 32) == 0;
                std::printf(", digest %s SHA-256(data)", ok ? "==" : "!=");
                if (!ok) ++bad;
                if (t == 0x5 || t == 0xD) std::printf("  \"%.*s\"", static_cast<int>(payload.size()), payload.data());
            } else {
                std::printf(", digest taken from the log (data is a description, not the measured bytes)");
            }
            std::printf("\n");
            sha::extend(soft[pcr], d.data());
        } else if (kind == "pcr") {
            unsigned pcr;
            std::string v;
            in >> pcr >> v;
            reported.push_back({pcr, unhex(v)});
        }
    }
    for (const auto& [pcr, v] : reported) {
        const bool ok = v.size() == 32 && std::memcmp(v.data(), soft[pcr], 32) == 0;
        if (!ok) ++bad;
        std::printf("PCR %2u: replay %s ... %s TPM %s ...\n", pcr, hex(soft[pcr], 8).c_str(), ok ? "==" : "!=",
                    hex(v.data(), 8).c_str());
    }
    std::printf("verifier: %d event(s), %zu PCR(s): %s\n", events, reported.size(),
                bad ? "REJECT (the log does not explain the PCR values)" : "ACCEPT");
    return bad ? 1 : 0;
}
