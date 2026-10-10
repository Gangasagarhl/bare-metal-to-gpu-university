// ssi.cc - DS403 cluster kernel: cluster-wide process list and remote run (F5-44).
#include "ssi.h"
#include "member.h"
#include "k.h"

namespace {
struct Task { uint16_t pid; char name[14]; };
// Each node's local task table. A teaching stand-in for your kernel's process table.
const Task tasks[] = { {1, "init"}, {2, "memberd"}, {3, "ssid"}, {7, "worker"} };
constexpr int NTASK = sizeof tasks / sizeof tasks[0];

struct PsRep { uint32_t req; uint16_t count; Task t[NTASK]; };
struct RunReq { uint32_t job; uint32_t n; };
struct RunRep { uint32_t job; uint32_t n; uint32_t result; uint32_t ticks; };

uint32_t cps_req = 0, cps_started = 0, cps_answered = 0;   // bit n: node n answered
bool cps_open = false;
constexpr uint32_t CPS_WAIT = 100;                          // ticks (1 s)

struct Pending { uint32_t job; uint8_t node; uint32_t sent; bool open; };
Pending jobs[8];
uint8_t load[MAX_NODES + 1];                               // jobs outstanding per node
constexpr uint32_t RUN_WAIT = 300;
}  // namespace

uint32_t count_primes(uint32_t n)
{
    uint32_t c = 0;
    for (uint32_t k = 2; k <= n; ++k) {
        bool prime = true;
        for (uint32_t d = 2; d * d <= k; ++d)
            if (k % d == 0) { prime = false; break; }
        if (prime) ++c;
    }
    return c;
}

void ssi_cps_start()
{
    ++cps_req;
    cps_started = now();
    cps_answered = 1u << g_node;
    cps_open = true;
    klog("cps #%u: asking every node", cps_req);
    kprintf("  GPID   NODE PID NAME\n");
    for (const Task& t : tasks) kprintf("  %u.%u    %u    %u   %s\n", g_node, t.pid, g_node, t.pid, t.name);
    net_send(0, M_PS_REQ, 0, &cps_req, sizeof cps_req);
}

void ssi_rrun(uint32_t job, uint32_t n)
{
    // Placement: the alive node (not me) with the fewest outstanding jobs; ties -> lowest id.
    uint32_t v = member_view();
    int best = -1;
    for (uint8_t k = 1; k <= MAX_NODES; ++k) {
        if (k == g_node || (v & (1u << k)) == 0) continue;
        if (best < 0 || load[k] < load[best]) best = k;
    }
    char s[32];
    member_format(v, s);
    if (best < 0) {
        klog("rrun job %u: no other node in view %s, running here", job, s);
        klog("job %u ran on node %u: primes<=%u = %u", job, g_node, n, count_primes(n));
        return;
    }
    for (Pending& p : jobs) {
        if (p.open) continue;
        p = Pending{job, static_cast<uint8_t>(best), now(), true};
        ++load[best];
        RunReq r{job, n};
        klog("rrun job %u (primes<=%u) -> node %u (view %s, load %u)", job, n, best, s, load[best]);
        net_send(static_cast<uint8_t>(best), M_RUN_REQ, 0, &r, sizeof r);
        return;
    }
}

void ssi_handle(const Msg& m)
{
    if (m.type == M_PS_REQ) {
        PsRep rep{};
        memcpy(&rep.req, m.data, sizeof rep.req);
        rep.count = NTASK;
        for (int i = 0; i < NTASK; ++i) rep.t[i] = tasks[i];
        net_send(m.from, M_PS_REP, 0, &rep, sizeof rep);
    } else if (m.type == M_PS_REP && cps_open) {
        PsRep rep;
        memcpy(&rep, m.data, sizeof rep);
        if (rep.req != cps_req) return;                      // a late answer to an old round
        cps_answered |= 1u << m.from;
        for (int i = 0; i < rep.count && i < NTASK; ++i)
            kprintf("  %u.%u    %u    %u   %s\n", m.from, rep.t[i].pid, m.from, rep.t[i].pid, rep.t[i].name);
    } else if (m.type == M_RUN_REQ) {
        RunReq r;
        memcpy(&r, m.data, sizeof r);
        uint32_t t0 = now();
        uint32_t res = count_primes(r.n);
        RunRep rep{r.job, r.n, res, now() - t0};
        klog("running job %u for node %u: primes<=%u = %u", r.job, m.from, r.n, res);
        net_send(m.from, M_RUN_REP, 0, &rep, sizeof rep);
    } else if (m.type == M_RUN_REP) {
        RunRep rep;
        memcpy(&rep, m.data, sizeof rep);
        for (Pending& p : jobs) {
            if (!p.open || p.job != rep.job) continue;
            p.open = false;
            --load[p.node];
            klog("job %u ran on node %u: primes<=%u = %u (remote compute %u ticks, round trip %u ticks)",
                 rep.job, m.from, rep.n, rep.result, rep.ticks, now() - p.sent);
        }
    }
}

void ssi_tick()
{
    uint32_t t = now();
    if (cps_open && t - cps_started >= CPS_WAIT) {
        cps_open = false;
        char s[32];
        member_format(cps_answered, s);
        klog("cps #%u done: answers from %s", cps_req, s);
        for (uint8_t k = 1; k <= MAX_NODES; ++k)
            if (k <= member_nodes() && (member_view() & (1u << k)) == 0 && (cps_answered & (1u << k)) == 0)
                klog("cps #%u: node %u not in the view, its processes are missing from the list", cps_req, k);
    }
    for (Pending& p : jobs) {
        if (p.open && t - p.sent >= RUN_WAIT) {
            p.open = false;
            --load[p.node];
            klog("job %u on node %u: no answer after %u ticks; result unknown", p.job, p.node, RUN_WAIT);
        }
    }
}
