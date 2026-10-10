// ordering_model.cpp - BR-06: a tiny teaching model of three memory-ordering rules.
// It enumerates every outcome each rule allows for the SB and MP litmus tests:
//   SC   sequential consistency: the threads' steps interleave in program order;
//   TSO  each thread has a first-in-first-out store buffer (the x86 rule, simplified);
//   WEAK a thread may reorder accesses to different variables unless an acquire,
//        release or seq_cst annotation forbids it (a simplified rule in the spirit of
//        Arm and RISC-V; NOT their official models, which are more subtle).
// It is our model, run on the host: evidence of what the rules imply, not of hardware.
#include <algorithm>
#include <cstdio>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

enum class Kind { Store, Load };
enum class Ord { Plain, Release, Acquire, SeqCst };

struct Op {
    Kind kind;
    int var;    // 0 = x/data, 1 = y/flag
    int arg;    // Store: value; Load: register number
    Ord ord;
};

using Prog = std::vector<Op>;
using Outcome = std::vector<int>;   // register values r0, r1

struct Test {
    const char* name;
    Prog t[2];
    int nregs;
    Outcome watched;
};

// ---------- SC and WEAK: interleave two per-thread sequences, memory updated at once
void interleave(const Prog& a, const Prog& b, size_t i, size_t j, std::vector<int> mem, Outcome regs,
                std::set<Outcome>& out)
{
    if (i == a.size() && j == b.size()) {
        out.insert(regs);
        return;
    }
    for (int side = 0; side < 2; ++side) {
        const Prog& p = side == 0 ? a : b;
        const size_t k = side == 0 ? i : j;
        if (k == p.size()) {
            continue;
        }
        std::vector<int> m = mem;
        Outcome r = regs;
        const Op& op = p[k];
        if (op.kind == Kind::Store) {
            m[static_cast<size_t>(op.var)] = op.arg;
        } else {
            r[static_cast<size_t>(op.arg)] = m[static_cast<size_t>(op.var)];
        }
        interleave(a, b, side == 0 ? i + 1 : i, side == 1 ? j + 1 : j, m, r, out);
    }
}

// WEAK: may `later` move before `earlier` (same thread, earlier in program order)?
bool may_swap(const Op& earlier, const Op& later)
{
    if (earlier.var == later.var) {
        return false;                                    // same variable: order kept
    }
    if (earlier.ord == Ord::SeqCst || later.ord == Ord::SeqCst) {
        return false;                                    // seq_cst: ordered with everything
    }
    if (earlier.kind == Kind::Load && earlier.ord == Ord::Acquire) {
        return false;                                    // nothing moves above an acquire
    }
    if (later.kind == Kind::Store && later.ord == Ord::Release) {
        return false;                                    // nothing moves below a release
    }
    return true;
}

std::vector<Prog> weak_orders(const Prog& p)
{
    std::vector<int> idx(p.size());
    for (size_t i = 0; i < idx.size(); ++i) {
        idx[i] = static_cast<int>(i);
    }
    std::vector<Prog> res;
    do {
        bool ok = true;
        for (size_t x = 0; x < idx.size() && ok; ++x) {
            for (size_t y = x + 1; y < idx.size() && ok; ++y) {
                if (idx[x] > idx[y] && !may_swap(p[static_cast<size_t>(idx[y])], p[static_cast<size_t>(idx[x])])) {
                    ok = false;
                }
            }
        }
        if (ok) {
            Prog q;
            for (int k : idx) {
                q.push_back(p[static_cast<size_t>(k)]);
            }
            res.push_back(q);
        }
    } while (std::next_permutation(idx.begin(), idx.end()));
    return res;
}

// ---------- TSO: store buffers
struct TsoState {
    size_t pc[2];
    std::vector<std::pair<int, int>> buf[2];   // (var, value), oldest first
    std::vector<int> mem;
    Outcome regs;
};

void tso(const Test& t, TsoState s, std::set<Outcome>& out)
{
    bool done = true;
    for (int th = 0; th < 2; ++th) {
        // step 1: drain the oldest buffered store of thread th to memory
        if (!s.buf[th].empty()) {
            done = false;
            TsoState n = s;
            n.mem[static_cast<size_t>(n.buf[th].front().first)] = n.buf[th].front().second;
            n.buf[th].erase(n.buf[th].begin());
            tso(t, n, out);
        }
        // step 2: execute thread th's next instruction
        if (s.pc[th] < t.t[th].size()) {
            done = false;
            const Op& op = t.t[th][s.pc[th]];
            if (op.ord == Ord::SeqCst && op.kind == Kind::Load && !s.buf[th].empty()) {
                continue;   // a seq_cst load waits until the buffer is empty (the fence)
            }
            TsoState n = s;
            n.pc[th] += 1;
            if (op.kind == Kind::Store) {
                n.buf[th].push_back({op.var, op.arg});
            } else {
                int v = n.mem[static_cast<size_t>(op.var)];
                for (const auto& e : n.buf[th]) {      // a thread sees its own newest store
                    if (e.first == op.var) {
                        v = e.second;
                    }
                }
                n.regs[static_cast<size_t>(op.arg)] = v;
            }
            tso(t, n, out);
        }
    }
    if (done) {
        out.insert(s.regs);
    }
}

std::string show(const std::set<Outcome>& s, const Outcome& watched, bool* allowed)
{
    std::string r;
    *allowed = s.count(watched) != 0;
    for (const Outcome& o : s) {
        r += " (";
        for (size_t i = 0; i < o.size(); ++i) {
            r += (i ? "," : "") + std::to_string(o[i]);
        }
        r += ")";
    }
    return r;
}

} // namespace

int main()
{
    const Op sx{Kind::Store, 0, 1, Ord::Plain}, sy{Kind::Store, 1, 1, Ord::Plain};
    const Op ly0{Kind::Load, 1, 0, Ord::Plain}, lx1{Kind::Load, 0, 1, Ord::Plain};
    auto sc = [](Op o) { o.ord = Ord::SeqCst; return o; };
    const Op rel_flag{Kind::Store, 1, 1, Ord::Release}, acq_flag{Kind::Load, 1, 0, Ord::Acquire};
    const Op ld_flag{Kind::Load, 1, 0, Ord::Plain}, ld_data{Kind::Load, 0, 1, Ord::Plain};
    const Test tests[] = {
        {"SB relaxed (x=1;r0=y | y=1;r1=x)", {{sx, ly0}, {sy, lx1}}, 2, {0, 0}},
        {"SB seq_cst", {{sc(sx), sc(ly0)}, {sc(sy), sc(lx1)}}, 2, {0, 0}},
        {"MP relaxed (data=1;flag=1 | r0=flag;r1=data)", {{sx, sy}, {ld_flag, ld_data}}, 2, {1, 0}},
        {"MP release/acquire", {{sx, rel_flag}, {acq_flag, ld_data}}, 2, {1, 0}},
    };
    for (const Test& t : tests) {
        std::printf("%s   watched outcome (%d,%d)\n", t.name, t.watched[0], t.watched[1]);
        std::set<Outcome> s_sc, s_tso, s_weak;
        interleave(t.t[0], t.t[1], 0, 0, {0, 0}, Outcome(static_cast<size_t>(t.nregs), 0), s_sc);
        TsoState start{{0, 0}, {}, {0, 0}, Outcome(static_cast<size_t>(t.nregs), 0)};
        tso(t, start, s_tso);
        for (const Prog& a : weak_orders(t.t[0])) {
            for (const Prog& b : weak_orders(t.t[1])) {
                interleave(a, b, 0, 0, {0, 0}, Outcome(static_cast<size_t>(t.nregs), 0), s_weak);
            }
        }
        const std::pair<const char*, const std::set<Outcome>*> rows[] = {{"SC  ", &s_sc}, {"TSO ", &s_tso}, {"WEAK", &s_weak}};
        for (const auto& row : rows) {
            bool allowed = false;
            const std::string list = show(*row.second, t.watched, &allowed);
            std::printf("  %s allows%-26s watched: %s\n", row.first, list.c_str(), allowed ? "ALLOWED" : "forbidden");
        }
    }
    return 0;
}
