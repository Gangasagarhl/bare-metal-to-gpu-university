// frame_demo.cc - F10-30 Listing 4: one heartbeat and one status text through the U-link
// encoder and parser: the bytes on the wire, payload truncation, a corrupted byte, noise
// between frames, and a lost frame seen through the sequence number.
#include <cstdio>
#include <cstring>
#include <vector>

#include "udialect.h"
#include "ulink.hpp"

namespace {

void dump(const char* title, const std::vector<std::uint8_t>& bytes)
{
    std::printf("%s (%zu bytes):", title, bytes.size());
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        std::printf("%s%02X", i % 16 == 0 ? "\n   " : " ", bytes[i]);
    }
    std::printf("\n");
}

bool lookup(std::uint32_t id, std::uint8_t& extra)
{
    const udialect::MsgInfo* info = udialect::find(id);
    if (info == nullptr) {
        return false;
    }
    extra = info->crcExtra;
    return true;
}

}  // namespace

int main()
{
    udialect::UHeartbeat hb;
    hb.type = 2;
    hb.autopilot = 3;
    hb.base_mode = 0x80;          // armed
    hb.custom_mode = 5;
    hb.system_status = 2;
    const auto f1 = ulink::encodeMsg(hb, 0, 1, 1);
    dump("heartbeat, seq 0, system 1, component 1", f1);

    udialect::UStatustext st;
    st.severity = 6;
    const char* text = "Mission accepted";
    std::memcpy(st.text.data(), text, std::strlen(text));
    const auto f2 = ulink::encodeMsg(st, 1, 1, 1);
    dump("status text, seq 1 (51-byte payload, trailing zeros not sent)", f2);

    ulink::Parser parser(lookup);
    std::vector<std::uint8_t> stream = {0x00, 0x55, 0x13};      // noise on the line
    stream.insert(stream.end(), f1.begin(), f1.end());
    auto bad = f1;
    bad[12] ^= 0x01;                                             // one bit flipped in the payload
    bad[4] = 2;                                                  // (seq 2)
    stream.insert(stream.end(), bad.begin(), bad.end());
    stream.insert(stream.end(), f2.begin(), f2.end());
    const auto f4 = ulink::encodeMsg(hb, 4, 1, 1);               // seq 3 never sent: "lost"
    stream.insert(stream.end(), f4.begin(), f4.end());

    int expectedSeq = -1;
    for (std::uint8_t b : stream) {
        if (auto fr = parser.feed(b)) {
            const udialect::MsgInfo* info = udialect::find(fr->msgid);
            std::printf("frame ok: seq %u from %u/%u, %s, payload %zu of %u bytes", fr->seq,
                        fr->sysid, fr->compid, info->name, fr->payload.size(), info->len);
            if (expectedSeq >= 0 && fr->seq != expectedSeq) {
                std::printf("  [gap: %d frame(s) missing]",
                            (fr->seq - expectedSeq + 256) % 256);
            }
            std::printf("\n");
            expectedSeq = (fr->seq + 1) % 256;
            if (fr->msgid == udialect::UStatustext::kId) {
                const auto m = udialect::UStatustext::unpack(fr->payload.data(), fr->payload.size());
                std::printf("   text: \"%.*s\"\n", 50, m.text.data());
            }
            if (fr->msgid == udialect::UHeartbeat::kId) {
                const auto m = udialect::UHeartbeat::unpack(fr->payload.data(), fr->payload.size());
                std::printf("   type %u, autopilot %u, armed %s, mode %u, status %u\n", m.type,
                            m.autopilot, (m.base_mode & 0x80) ? "yes" : "no", m.custom_mode,
                            m.system_status);
            }
        }
    }
    std::printf("parser: %u frames ok, %u CRC errors, %u unknown ids, %u noise bytes skipped\n",
                parser.stats.framesOk, parser.stats.crcErrors, parser.stats.unknownId,
                parser.stats.bytesSkipped);
    return 0;
}
