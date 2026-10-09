// cluster.h - a whole Raft cluster inside one process: servers, the simulated
// network, a script of faults, clients, a trace, and the safety checker.
#pragma once
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "raft.h"
#include "sim.h"

namespace raft {

// One client operation as the client saw it (for the linearizability check).
struct HistoryOp
{
    int process = 0;
    bool isPut = false;
    int value = 0;        // put: value written; get: value returned
    Time invoke = 0;
    Time response = -1;   // -1: no reply ever arrived (outcome unknown)
};

class Cluster : public Env
{
public:
    enum class Trace { None, Events };

    Cluster(int servers, std::uint64_t seed, Options opt = {}, Faults faults = {},
            sim::NetOptions net = {})
        : rng_(seed), net_(net, rng_), opt_(opt), faults_(faults)
    {
        Config boot;
        for (int i = 1; i <= servers; ++i) {
            boot.members.push_back(i);
        }
        for (int i = 1; i <= servers; ++i) {
            nodes_[i] = std::make_unique<Node>(i, boot, opt_, faults_, *this);
        }
    }

    // A new, empty server that is in no configuration yet (it waits to be added).
    void addSpareServer(int id)
    {
        nodes_[id] = std::make_unique<Node>(id, Config{}, opt_, faults_, *this);
    }

    Trace traceLevel = Trace::Events;

    // ---- the script: things that happen at given times ----
    void at(Time t, std::function<void(Cluster&)> action) { script_.emplace(t, std::move(action)); }

    void crash(int n)
    {
        say(n, "*** CRASH ***");
        node(n).crash();
    }
    void restart(int n) { node(n).restart(); }
    void isolate(int n)
    {
        say(n, "*** network: cut off from all other servers ***");
        isolated_.insert(n);
        for (const auto& [id, _] : nodes_) {
            if (id != n) {
                net_.block(n, id);
            }
        }
    }
    void reconnect(int n)  // to every server that is not itself isolated
    {
        say(n, "*** network: reconnected ***");
        isolated_.erase(n);
        for (const auto& [id, _] : nodes_) {
            if (isolated_.count(id) == 0) {
                net_.unblock(n, id);
            }
        }
    }
    void block(int a, int b) { net_.block(a, b); }
    void unblock(int a, int b) { net_.unblock(a, b); }
    void partition(const std::vector<int>& side)
    {
        net_.healAll();
        isolated_.clear();
        for (const auto& [a, _] : nodes_) {
            for (const auto& [b, __] : nodes_) {
                const bool inA = std::find(side.begin(), side.end(), a) != side.end();
                const bool inB = std::find(side.begin(), side.end(), b) != side.end();
                if (inA != inB) {
                    net_.block(a, b);
                }
            }
        }
        say(0, "*** network: partition " + listIds(side) + " | rest ***");
    }
    void heal()
    {
        net_.healAll();
        isolated_.clear();
        say(0, "*** network: healed ***");
    }

    // A scripted client request (client id 100) sent to server n.
    void submit(int n, OpType type, char key = 'x', int value = 0)
    {
        Message m;
        m.type = MsgType::ClientRequest;
        m.from = 100;
        m.to = n;
        m.op.type = type;
        m.op.key = key;
        m.op.value = value;
        m.op.request = ++scriptRequests_;
        HistoryOp h;
        h.process = 200 + scriptRequests_;  // each scripted request is its own process
        h.isPut = type == OpType::Put;
        h.value = value;
        h.invoke = now_;
        scriptHistory_[scriptRequests_] = history_.size();
        history_.push_back(h);
        say(0, std::string("client -> n") + std::to_string(n) + ": " +
                   (h.isPut ? std::string("put ") + key + "=" + std::to_string(value)
                            : std::string("get ") + key));
        net_.send(now_, m);
    }
    void changeConfig(int n, std::vector<int> members)
    {
        Message m;
        m.type = MsgType::ClientRequest;
        m.from = 100;
        m.to = n;
        m.op.type = OpType::Config;
        say(0, "client -> n" + std::to_string(n) + ": change configuration to " + listIds(members));
        m.op.config.members = std::move(members);
        m.op.request = ++scriptRequests_;
        net_.send(now_, m);
    }

    // ---- random clients for the testing chapter ----
    void startClients(int count, int maxOps, Time timeout = 400)
    {
        clientTimeout_ = timeout;
        maxOps_ = maxOps;
        for (int i = 0; i < count; ++i) {
            clients_.push_back(ClientSlot{});
            clients_.back().process = nextProcess_++;
        }
    }

    // ---- run ----
    void runUntil(Time end)
    {
        while (now_ < end) {
            ++now_;
            for (const Message& m : net_.due(now_)) {
                deliver(m);
            }
            auto range = script_.equal_range(now_);
            std::vector<std::function<void(Cluster&)>> actions;
            for (auto it = range.first; it != range.second; ++it) {
                actions.push_back(it->second);
            }
            for (auto& a : actions) {
                a(*this);
            }
            for (auto& [id, n] : nodes_) {
                n->tick();
            }
            stepClients();
            if (now_ % 50 == 0) {
                checkLogMatching();
            }
        }
        checkLogMatching();
    }

    // ---- Env ----
    Time now() const override { return now_; }
    sim::Rng& rng() override { return rng_; }
    void send(const Message& m) override { net_.send(now_, m); }
    void trace(int node, int term, Role role, const std::string& text) override
    {
        if (traceLevel == Trace::Events) {
            std::printf("%5lld ms  n%d  term %-2d %-9s  %s\n", static_cast<long long>(now_), node,
                        term, roleName(role), text.c_str());
        }
    }
    void becameLeader(int n, int term) override
    {
        auto [it, fresh] = leaders_.emplace(term, n);
        if (!fresh && it->second != n) {
            violation("Election Safety: n" + std::to_string(it->second) + " and n" +
                      std::to_string(n) + " are both leader of term " + std::to_string(term));
        }
        // Leader Completeness: every entry some server applied must be in my log.
        for (const auto& [index, e] : appliedAt_) {
            const Node& me = node(n);
            if (index > me.lastIndex() || me.termAt(index) != e.term) {
                violation("Leader Completeness: new leader n" + std::to_string(n) + " of term " +
                          std::to_string(term) + " lacks committed entry " +
                          std::to_string(index) + " (" + describe(e) + ")");
            }
        }
    }
    void applied(int n, int index, const Entry& e) override
    {
        auto [it, fresh] = appliedAt_.emplace(index, e);
        if (!fresh && !sameEntry(it->second, e)) {
            violation("State Machine Safety: entry " + std::to_string(index) + " applied as '" +
                      describe(it->second) + "' earlier, and as '" + describe(e) + "' by n" +
                      std::to_string(n));
        }
    }

    // ---- inspection ----
    Node& node(int id) { return *nodes_.at(id); }
    const Node& node(int id) const { return *nodes_.at(id); }
    const std::vector<std::string>& violations() const { return violations_; }
    const std::vector<HistoryOp>& history() const { return history_; }
    long dropped() const { return net_.dropped(); }
    int leaderCount() const { return static_cast<int>(leaders_.size()); }

    void printLogs() const
    {
        for (const auto& [id, n] : nodes_) {
            std::printf("  n%d %-4s term %-2d commit %-2d log:", id, n->up() ? "up" : "DOWN",
                        n->term(), n->commitIndex());
            for (int i = 1; i <= n->lastIndex(); ++i) {
                std::printf(" [%d %s]", i, describe(n->log()[i - 1]).c_str());
            }
            std::printf("\n");
        }
    }
    void printStates() const
    {
        for (const auto& [id, n] : nodes_) {
            std::printf("  n%d state machine:", id);
            for (const auto& [k, v] : n->state()) {
                std::printf(" %c=%d", k, v);
            }
            std::printf("%s\n", n->state().empty() ? " (empty)" : "");
        }
    }
    void printViolations() const
    {
        if (violations_.empty()) {
            std::printf("checker: no safety violation\n");
        }
        for (const auto& v : violations_) {
            std::printf("checker: VIOLATION %s\n", v.c_str());
        }
    }

private:
    struct ClientSlot
    {
        int process = 0;
        bool busy = false;
        int target = 1;
        Time deadline = 0;
        Time nextStart = 0;
        int request = 0;
        std::size_t historyIndex = 0;
        Entry op;
    };

    void say(int n, const std::string& text)
    {
        if (traceLevel == Trace::Events) {
            if (n == 0) {
                std::printf("%5lld ms  %s\n", static_cast<long long>(now_), text.c_str());
            } else {
                std::printf("%5lld ms  n%d  %s\n", static_cast<long long>(now_), n, text.c_str());
            }
        }
    }

    void violation(const std::string& v)
    {
        for (const auto& old : violations_) {
            if (old == v) {
                return;
            }
        }
        violations_.push_back(v);
        say(0, "!!! checker: " + v);
    }

    void deliver(const Message& m)
    {
        if (m.to >= 100) {
            clientReply(m);
        } else if (nodes_.count(m.to)) {
            nodes_.at(m.to)->receive(m);
        }
    }

    // Log Matching: same index and term => identical logs up to that index.
    void checkLogMatching()
    {
        for (const auto& [a, na] : nodes_) {
            for (const auto& [b, nb] : nodes_) {
                if (a >= b) {
                    continue;
                }
                const int upto = std::min(na->lastIndex(), nb->lastIndex());
                for (int i = upto; i >= 1; --i) {
                    if (na->termAt(i) != nb->termAt(i)) {
                        continue;
                    }
                    for (int j = 1; j <= i; ++j) {
                        if (!sameEntry(na->log()[j - 1], nb->log()[j - 1])) {
                            violation("Log Matching: n" + std::to_string(a) + " and n" +
                                      std::to_string(b) + " both have term " +
                                      std::to_string(na->termAt(i)) + " at entry " +
                                      std::to_string(i) + ", but their logs differ at entry " +
                                      std::to_string(j));
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    void stepClients()
    {
        for (auto& c : clients_) {
            if (c.busy && now_ >= c.deadline) {
                // No answer: the outcome is unknown. This process gives up for good and
                // a fresh process takes its place (one outstanding operation each).
                c.busy = false;
                c.process = nextProcess_++;
                c.nextStart = now_ + 10;
            }
            if (!c.busy && now_ >= c.nextStart && opsStarted_ < maxOps_) {
                c.busy = true;
                ++opsStarted_;
                c.op = Entry{};
                c.op.key = 'x';
                if (rng_.percent(50)) {
                    c.op.type = OpType::Put;
                    c.op.value = ++lastValue_;
                } else {
                    c.op.type = OpType::Get;
                }
                c.op.request = ++c.request;
                c.deadline = now_ + clientTimeout_;
                c.target = static_cast<int>(rng_.range(1, static_cast<Time>(nodes_.size())));
                HistoryOp h;
                h.process = c.process;
                h.isPut = c.op.type == OpType::Put;
                h.value = c.op.value;
                h.invoke = now_;
                c.historyIndex = history_.size();
                history_.push_back(h);
                sendClient(c);
            }
        }
    }

    void sendClient(const ClientSlot& c)
    {
        Message m;
        m.type = MsgType::ClientRequest;
        m.from = c.process;
        m.to = c.target;
        m.op = c.op;
        net_.send(now_, m);
    }

    void clientReply(const Message& m)
    {
        if (m.to == 100) {  // the scripted client: report, and record in the history
            const std::string what = describe(m.op).substr(describe(m.op).find(' ') + 1);
            if (!m.ok) {
                say(0, "client <- n" + std::to_string(m.from) + ": " + what + " refused, " +
                           (m.leaderHint ? "leader is n" + std::to_string(m.leaderHint)
                                         : std::string("no leader known")));
                return;
            }
            const bool get = m.op.type == OpType::Get;
            say(0, "client <- n" + std::to_string(m.from) + ": " + what + " done" +
                       (get ? ", value " + std::to_string(m.result) : ""));
            const auto it = scriptHistory_.find(m.op.request);
            if (it != scriptHistory_.end() && history_[it->second].response < 0) {
                history_[it->second].response = now_;
                if (get) {
                    history_[it->second].value = m.result;
                }
            }
            return;
        }
        for (auto& c : clients_) {
            if (!c.busy || c.process != m.to || m.op.request != c.op.request) {
                continue;  // a reply for an operation this process gave up on
            }
            if (!m.ok) {  // not the leader: try the hinted server, or another one
                c.target = m.leaderHint != 0 ? m.leaderHint
                                             : static_cast<int>(rng_.range(1, static_cast<Time>(nodes_.size())));
                sendClient(c);
                return;
            }
            HistoryOp& h = history_[c.historyIndex];
            h.response = now_;
            if (!h.isPut) {
                h.value = m.result;
            }
            c.busy = false;
            c.nextStart = now_ + rng_.range(0, 30);
            return;
        }
    }

    sim::Rng rng_;
    sim::Network<Message> net_;
    Options opt_;
    Faults faults_;
    std::map<int, std::unique_ptr<Node>> nodes_;
    std::multimap<Time, std::function<void(Cluster&)>> script_;
    std::set<int> isolated_;
    Time now_ = 0;
    int scriptRequests_ = 0;
    std::map<int, std::size_t> scriptHistory_;  // scripted request -> history entry
    std::map<int, int> leaders_;     // term -> leader
    std::map<int, Entry> appliedAt_; // index -> entry, as first applied anywhere
    std::vector<std::string> violations_;
    std::vector<ClientSlot> clients_;
    std::vector<HistoryOp> history_;
    int nextProcess_ = 101;
    int opsStarted_ = 0;
    int maxOps_ = 0;
    int lastValue_ = 0;
    Time clientTimeout_ = 400;
};

} // namespace raft
