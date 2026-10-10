// naive.cc - DS403 cluster kernel: leader = lowest node id in MY membership view.
// The leader applies a command at once, answers the client, then tells the others.
// When a view grows and I am its leader, I push my whole table to everyone ("I win").
#include "naive.h"
#include "k.h"
#include "member.h"

namespace {
uint8_t leader = 0;
uint32_t seen_view = 0;
uint16_t next_req = 1;
uint32_t applied = 0;

struct Update { uint8_t full; uint8_t n; Entry e; char keys[8][8]; char vals[8][8]; };
struct Reply { uint16_t reqid; uint8_t ok; };

uint8_t lowest(uint32_t view)
{
    for (uint8_t k = 1; k <= MAX_NODES; ++k)
        if (view & (1u << k)) return k;
    return 0;
}

void apply_and_tell(const Entry& e)
{
    bool ok = sm_apply(++applied, e);
    char s[48];
    sm_format(e, s);
    klog("naive leader: apply %s%s", s, (e.kind == K_CAS && !ok) ? " -> refused" : "");
    Update u{};
    u.e = e;
    net_send(0, M_APPEND, 0, &u, sizeof u);
    if (e.client == g_node) klog("client: request %u (%s): %s", e.reqid, s, ok ? "OK" : "refused");
    else if (e.client != 0) {
        Reply r{e.reqid, static_cast<uint8_t>(ok)};
        net_send(e.client, M_CLIENT_REP, 0, &r, sizeof r);
    }
}
}  // namespace

void naive_init() { klog("naive: leader = lowest node id in my view (no majority)"); }

void naive_tick()
{
    uint32_t v = member_view();
    if (v == seen_view) return;
    bool grew = (v & ~seen_view) != 0 && seen_view != 0;
    seen_view = v;
    uint8_t l = lowest(v);
    char s[32];
    member_format(v, s);
    if (l != leader) klog("naive: leader is now node %u (view %s)%s", l, s, l == g_node ? " -- that is me" : "");
    leader = l;
    if (grew && leader == g_node) {                     // push my table to the bigger group
        Update u{};
        u.full = 1;
        u.n = static_cast<uint8_t>(sm_export(u.keys, u.vals));
        klog("naive: view grew to %s and I lead it: sending my whole table", s);
        net_send(0, M_APPEND, 0, &u, sizeof u);
    }
}

void naive_handle(const Msg& m)
{
    if (m.type == M_CLIENT && leader == g_node) {
        Entry e;
        memcpy(&e, m.data, sizeof e);
        apply_and_tell(e);
    } else if (m.type == M_APPEND && m.from == leader) {
        Update u;
        memcpy(&u, m.data, sizeof u);
        if (u.full) {
            sm_restore(u.keys, u.vals, u.n);
        } else {
            bool ok = sm_apply(++applied, u.e);
            char s[48];
            sm_format(u.e, s);
            klog("naive follower: apply %s from leader %u%s", s, m.from, (u.e.kind == K_CAS && !ok) ? " -> refused" : "");
        }
    } else if (m.type == M_CLIENT_REP) {
        Reply r;
        memcpy(&r, m.data, sizeof r);
        klog("client: request %u: %s (answered by node %u)", r.reqid, r.ok ? "OK" : "refused", m.from);
    }
}

void naive_client(Entry e)
{
    e.client = g_node;
    e.reqid = next_req++;
    char s[48];
    sm_format(e, s);
    klog("client: request %u: %s (leader I know: %u)", e.reqid, s, leader);
    if (leader == g_node) apply_and_tell(e);
    else net_send(leader, M_CLIENT, 0, &e, sizeof e);
}
