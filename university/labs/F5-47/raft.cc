// raft.cc - DS403 cluster kernel: Raft election + replication, client requests, scheduler.
// Rule numbers in comments refer to the Raft paper's sections (Ongaro and Ousterhout).
#include "raft.h"
#include "k.h"
#include "member.h"
#include "sm.h"

namespace {
enum Role : uint8_t { FOLLOWER, CANDIDATE, LEADER };
const char* role_name[] = {"follower", "candidate", "leader"};

uint8_t n_nodes = 3;
uint32_t term = 0;
uint8_t voted_for = 0, leader = 0;
Role role = FOLLOWER;
Entry log[128];
uint32_t nlog = 0, commit = 0, applied = 0;   // log index i is log[i - 1]
uint32_t next_idx[MAX_NODES + 1], match_idx[MAX_NODES + 1];
uint32_t votes = 0, deadline = 0, last_beat = 0;
constexpr uint32_t BEAT = 20;                 // leader heartbeat (append) period, ticks

struct VoteReq { uint32_t last_index, last_term; };
struct VoteRep { uint8_t granted; };
struct Append { uint32_t prev_index, prev_term, leader_commit; uint8_t n; Entry e[8]; };
struct AppendRep { uint8_t success; uint32_t match; };
struct Client { Entry e; };
struct ClientRep { uint16_t reqid; uint8_t ok; uint32_t index; };
struct JobDone { uint32_t job, result; };

// The client side (this node's own requests).
uint16_t next_req = 1;
struct Pending { bool open; uint16_t reqid; uint32_t until; char what[48]; };
Pending pend[4];

// Jobs this node is running (assigned to it by applied JOB_ASSIGN entries).
struct Running { bool on; uint32_t job, end, done_sent; };
Running run[MAX_JOBS + 1];
uint32_t proposed_assign[MAX_JOBS + 1];       // leader: log index of its last JOB_ASSIGN

uint32_t last_term() { return nlog == 0 ? 0 : log[nlog - 1].term; }

void reset_deadline() { deadline = now() + 100 + 30 * g_node + rand_u32() % 40; }

void become_follower(uint32_t t, const char* why)
{
    if (role != FOLLOWER || t != term) klog("raft: term %u -> %u (%s), %s -> follower", term, t, why, role_name[role]);
    if (t != term) voted_for = 0;
    if (role != FOLLOWER) reset_deadline();       // a fresh election timer from now
    term = t;
    role = FOLLOWER;
}

void append_local(const Entry& e)
{
    if (nlog == sizeof log / sizeof log[0]) { klog("raft: log full"); return; }
    log[nlog++] = e;
    log[nlog - 1].term = term;
    char s[48];
    sm_format(e, s);
    klog("raft: leader appends index %u (term %u): %s", nlog, term, s);
}

void send_append(uint8_t to)
{
    Append a{};
    a.prev_index = next_idx[to] - 1;
    a.prev_term = a.prev_index == 0 ? 0 : log[a.prev_index - 1].term;
    a.leader_commit = commit;
    for (uint32_t i = next_idx[to]; i <= nlog && a.n < 8; ++i) a.e[a.n++] = log[i - 1];
    net_send(to, M_APPEND, term, &a, sizeof a);
}

void client_done(uint16_t reqid, bool ok, uint32_t index)
{
    for (Pending& p : pend) {
        if (!p.open || p.reqid != reqid) continue;
        p.open = false;
        klog("client: request %u (%s) committed at index %u: %s", reqid, p.what, index, ok ? "OK" : "refused by the state machine");
    }
}

void apply_committed()
{
    while (applied < commit) {
        ++applied;
        const Entry& e = log[applied - 1];
        bool ok = sm_apply(applied, e);
        char s[48];
        sm_format(e, s);
        klog("apply %u (term %u): %s%s", applied, e.term, s, (e.kind == K_CAS && !ok) ? " -> refused" : "");
        if (e.client == g_node) client_done(e.reqid, ok, applied);
        else if (e.client != 0 && role == LEADER) {
            ClientRep r{e.reqid, static_cast<uint8_t>(ok), applied};
            net_send(e.client, M_CLIENT_REP, term, &r, sizeof r);
        }
        if (e.kind == K_JOB_ASSIGN && e.b == g_node && sm_job(e.a).state == J_ASSIGNED) {
            run[e.a] = Running{true, e.a, now() + sm_job(e.a).duration, 0};
            klog("worker: starting job %u (%u ticks)", e.a, sm_job(e.a).duration);
        }
        if (e.kind == K_JOB_DONE && e.a <= MAX_JOBS) run[e.a].on = false;
    }
}

void leader_scheduler()
{
    // Place every submitted job, and every job whose node left the membership view.
    uint32_t view = member_view();
    uint8_t load[MAX_NODES + 1] = {};
    for (uint32_t j = 1; j <= MAX_JOBS; ++j)
        if (sm_job(j).state == J_ASSIGNED) ++load[sm_job(j).node];
    for (uint32_t j = 1; j <= MAX_JOBS; ++j) {
        const Job& x = sm_job(j);
        bool orphan = x.state == J_ASSIGNED && (view & (1u << x.node)) == 0;
        if (x.state != J_SUBMITTED && !orphan) continue;
        if (proposed_assign[j] > applied) continue;          // our last proposal not applied yet
        int best = -1;
        for (uint8_t k = 1; k <= n_nodes; ++k)
            if ((view & (1u << k)) && (best < 0 || load[k] < load[best])) best = k;
        if (best < 0) return;
        if (orphan) klog("scheduler: node %u left the view; job %u must run again", x.node, j);
        Entry e{};
        e.kind = K_JOB_ASSIGN; e.a = j; e.b = static_cast<uint32_t>(best);
        append_local(e);
        proposed_assign[j] = nlog;
        ++load[best];
    }
}

void worker_tick()
{
    uint32_t t = now();
    for (uint32_t j = 1; j <= MAX_JOBS; ++j) {
        Running& r = run[j];
        if (!r.on || t < r.end) continue;
        if (r.done_sent != 0 && t - r.done_sent < 100) continue;   // resend until applied
        JobDone d{j, count_primes_upto(j * 5000)};
        if (r.done_sent == 0) klog("worker: job %u finished, result %u", j, d.result);
        r.done_sent = t;
        if (role == LEADER) {
            Entry e{};
            e.kind = K_JOB_DONE; e.a = j; e.b = d.result;
            append_local(e);
        } else {
            net_send(0, M_JOB_DONE, term, &d, sizeof d);
        }
    }
}
}  // namespace

uint32_t count_primes_upto(uint32_t n)
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

void raft_init(uint8_t nodes)
{
    n_nodes = nodes;
    rand_seed(0x9E3779B9u * g_node);
    reset_deadline();
    klog("raft: %u nodes, majority %u, follower in term 0", n_nodes, n_nodes / 2 + 1);
}

bool raft_is_leader() { return role == LEADER; }

void raft_tick()
{
    uint32_t t = now();
    if (role != LEADER && t >= deadline) {                  // 5.2: start an election
        ++term;
        role = CANDIDATE;
        voted_for = g_node;
        votes = 1u << g_node;
        leader = 0;
        reset_deadline();
        klog("raft: election timeout, candidate for term %u", term);
        VoteReq v{nlog, last_term()};
        net_send(0, M_VOTE_REQ, term, &v, sizeof v);
    }
    if (role == LEADER) {
        if (t - last_beat >= BEAT) {
            last_beat = t;
            for (uint8_t k = 1; k <= n_nodes; ++k)
                if (k != g_node) send_append(k);
        }
        // 5.3 and 5.4.2: commit the highest index stored on a majority, if from this term.
        for (uint32_t i = nlog; i > commit; --i) {
            uint8_t count = 1;
            for (uint8_t k = 1; k <= n_nodes; ++k)
                if (k != g_node && match_idx[k] >= i) ++count;
            if (count > n_nodes / 2 && log[i - 1].term == term) { commit = i; break; }
        }
        leader_scheduler();
    }
    apply_committed();
    worker_tick();
    for (Pending& p : pend)
        if (p.open && t >= p.until) {
            p.open = false;
            klog("client: request %u (%s): no answer in time; outcome UNKNOWN", p.reqid, p.what);
        }
}

void raft_handle(const Msg& m)
{
    bool raft_msg = m.type == M_VOTE_REQ || m.type == M_VOTE_REP || m.type == M_APPEND || m.type == M_APPEND_REP;
    if (raft_msg && m.term > term) become_follower(m.term, "saw a higher term");   // 5.1
    switch (m.type) {
    case M_VOTE_REQ: {
        VoteReq v;
        memcpy(&v, m.data, sizeof v);
        bool up_to_date = v.last_term > last_term() || (v.last_term == last_term() && v.last_index >= nlog);
        VoteRep r{0};
        if (m.term == term && (voted_for == 0 || voted_for == m.from) && up_to_date) {   // 5.2, 5.4.1
            voted_for = m.from;
            r.granted = 1;
            reset_deadline();
        }
        klog("raft: vote for node %u in term %u: %s", m.from, m.term, r.granted ? "yes" : "no");
        net_send(m.from, M_VOTE_REP, term, &r, sizeof r);
        break;
    }
    case M_VOTE_REP: {
        if (role != CANDIDATE || m.term != term || m.data[0] == 0) break;
        votes |= 1u << m.from;
        if (member_count(votes) > n_nodes / 2) {
            role = LEADER;
            leader = g_node;
            klog("raft: LEADER for term %u with votes from %u nodes", term, member_count(votes));
            for (uint8_t k = 1; k <= n_nodes; ++k) { next_idx[k] = nlog + 1; match_idx[k] = 0; }
            Entry e{};
            e.kind = K_NOOP;                              // commits earlier terms' entries (5.4.2)
            append_local(e);
            last_beat = now() - BEAT;
        }
        break;
    }
    case M_APPEND: {
        Append a;
        memcpy(&a, m.data, sizeof a);
        AppendRep r{0, 0};
        if (m.term < term) { net_send(m.from, M_APPEND_REP, term, &r, sizeof r); break; }
        if (role != FOLLOWER) become_follower(m.term, "a leader exists");
        if (leader != m.from) klog("raft: following leader %u in term %u", m.from, m.term);
        leader = m.from;
        reset_deadline();
        if (a.prev_index > nlog || (a.prev_index > 0 && log[a.prev_index - 1].term != a.prev_term)) {
            r.match = nlog < a.prev_index ? nlog : a.prev_index - 1;   // 5.3: consistency check failed
            net_send(m.from, M_APPEND_REP, term, &r, sizeof r);
            break;
        }
        for (uint8_t i = 0; i < a.n; ++i) {
            uint32_t idx = a.prev_index + 1 + i;
            if (idx <= nlog && log[idx - 1].term != a.e[i].term) {
                klog("raft: conflict at index %u (mine term %u, leader's term %u): truncating %u entries",
                     idx, log[idx - 1].term, a.e[i].term, nlog - idx + 1);
                nlog = idx - 1;
            }
            if (idx > nlog) log[nlog++] = a.e[i];
        }
        uint32_t last_new = a.prev_index + a.n;
        if (a.leader_commit > commit) commit = a.leader_commit < last_new ? a.leader_commit : last_new;
        r.success = 1;
        r.match = last_new;
        net_send(m.from, M_APPEND_REP, term, &r, sizeof r);
        break;
    }
    case M_APPEND_REP: {
        if (role != LEADER || m.term != term) break;
        AppendRep r;
        memcpy(&r, m.data, sizeof r);
        if (r.success) {
            if (r.match > match_idx[m.from]) match_idx[m.from] = r.match;
            next_idx[m.from] = match_idx[m.from] + 1;
        } else {
            next_idx[m.from] = r.match + 1;
            send_append(m.from);
        }
        break;
    }
    case M_CLIENT: {
        if (role != LEADER) break;                       // only the leader takes commands
        Client c;
        memcpy(&c, m.data, sizeof c);
        append_local(c.e);
        break;
    }
    case M_CLIENT_REP: {
        ClientRep r;
        memcpy(&r, m.data, sizeof r);
        client_done(r.reqid, r.ok != 0, r.index);
        break;
    }
    case M_JOB_DONE: {
        if (role != LEADER) break;
        JobDone d;
        memcpy(&d, m.data, sizeof d);
        if (sm_job(d.job).state == J_DONE) break;
        for (uint32_t i = applied + 1; i <= nlog; ++i)   // already proposed and waiting?
            if (log[i - 1].kind == K_JOB_DONE && log[i - 1].a == d.job) return;
        Entry e{};
        e.kind = K_JOB_DONE; e.a = d.job; e.b = d.result;
        append_local(e);
        break;
    }
    default:
        break;
    }
}

void raft_client(Entry e, uint32_t wait)
{
    e.client = g_node;
    e.reqid = next_req++;
    for (Pending& p : pend) {
        if (p.open) continue;
        p.open = true;
        p.reqid = e.reqid;
        p.until = now() + wait;
        sm_format(e, p.what);
        klog("client: request %u: %s (I am %s, leader I know: %u)", e.reqid, p.what, role_name[role], leader);
        break;
    }
    if (role == LEADER) append_local(e);
    else net_send(0, M_CLIENT, term, &e, sizeof e);
}
