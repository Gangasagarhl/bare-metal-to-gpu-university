// member.cc - DS403 cluster kernel: heartbeat membership with a suspicion timeout.
// A node is in the view while a message from it arrived within `timeout` ticks.
// This detector can be wrong in both directions (F5-11): it only says "heard recently".
#include "member.h"
#include "k.h"

namespace {
uint8_t n_nodes = 3;
uint32_t hb_period = 10, hb_timeout = 50;
uint32_t last_heard[MAX_NODES + 1];
bool ever_heard[MAX_NODES + 1];
uint32_t last_sent = 0;
uint32_t view = 0, view_no = 0;
}

void member_init(uint8_t nodes, uint32_t period, uint32_t timeout)
{
    n_nodes = nodes; hb_period = period; hb_timeout = timeout;
    view = 1u << g_node;
    view_no = 1;
    char s[32];
    member_format(view, s);
    klog("view %u: %s (start)", view_no, s);
}

void member_heard(const Msg& m)
{
    if (m.from == 0 || m.from > MAX_NODES) return;
    last_heard[m.from] = now();
    ever_heard[m.from] = true;
}

void member_tick()
{
    uint32_t t = now();
    if (t - last_sent >= hb_period) {
        uint32_t up = t;
        net_send(0, M_HEARTBEAT, 0, &up, sizeof up);
        last_sent = t;
    }
    uint32_t v = 1u << g_node;
    for (uint8_t n = 1; n <= n_nodes; ++n)
        if (n != g_node && ever_heard[n] && t - last_heard[n] <= hb_timeout) v |= 1u << n;
    if (v != view) {
        char a[32], b[32];
        member_format(view, a);
        member_format(v, b);
        view = v;
        ++view_no;
        klog("view %u: %s (was %s)", view_no, b, a);
    }
}

uint32_t member_view() { return view; }
uint32_t member_view_number() { return view_no; }
uint8_t member_nodes() { return n_nodes; }

uint8_t member_count(uint32_t v)
{
    uint8_t c = 0;
    for (; v != 0; v &= v - 1) ++c;
    return c;
}

void member_format(uint32_t v, char* out)
{
    int k = 0;
    out[k++] = '{';
    for (uint8_t n = 1; n <= MAX_NODES; ++n) {
        if ((v & (1u << n)) == 0) continue;
        if (out[k - 1] != '{') out[k++] = ',';
        out[k++] = static_cast<char>('0' + n);
    }
    out[k++] = '}';
    out[k] = '\0';
}
