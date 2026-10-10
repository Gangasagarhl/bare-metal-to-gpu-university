// msg.cc - DS403 cluster kernel: pack and unpack cluster messages.
#include "msg.h"
#include "e1000.h"
#include "k.h"

namespace {
uint8_t frame[1514];
uint8_t in[1514];
}

bool net_init()
{
    if (!e1000_init()) return false;
    const uint8_t* m = e1000_mac();
    klog("e1000 up, MAC %02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
    return true;
}

bool net_send(uint8_t to, uint8_t type, uint32_t term, const void* payload, uint16_t len)
{
    if (len > MAX_PAYLOAD) return false;
    memset(frame, 0xFF, 6);                          // destination: broadcast
    memcpy(frame + 6, e1000_mac(), 6);
    frame[12] = ETHERTYPE_DS403 >> 8;                // EtherType is big-endian on the wire
    frame[13] = ETHERTYPE_DS403 & 0xFF;
    Hdr h{'D', 1, g_node, to, type, 0, len, term};
    memcpy(frame + 14, &h, sizeof h);
    memcpy(frame + 14 + sizeof h, payload, len);
    uint16_t total = static_cast<uint16_t>(14 + sizeof h + len);
    if (total < 60) { memset(frame + total, 0, 60 - total); total = 60; }   // minimum frame
    for (int tries = 0; tries < 1000; ++tries)
        if (e1000_send(frame, total)) return true;
    return false;
}

bool net_poll(Msg& m)
{
    for (;;) {
        int n = e1000_poll(in, sizeof in);
        if (n == 0) return false;
        if (n < 14 + static_cast<int>(sizeof(Hdr))) continue;
        if (in[12] != (ETHERTYPE_DS403 >> 8) || in[13] != (ETHERTYPE_DS403 & 0xFF)) continue;
        Hdr h;
        memcpy(&h, in + 14, sizeof h);
        if (h.magic != 'D' || h.version != 1 || h.from == g_node) continue;
        if (h.to != 0 && h.to != g_node) continue;   // addressed to another node
        if (h.len > MAX_PAYLOAD || 14 + sizeof h + h.len > static_cast<unsigned>(n)) continue;
        m.from = h.from; m.to = h.to; m.type = h.type; m.term = h.term; m.len = h.len;
        memcpy(m.data, in + 14 + sizeof h, h.len);
        return true;
    }
}
