// two_leaders_sim.hpp - a small discrete-event simulation of three nodes that elect a leader
// with heartbeats and timeouts (DS301 forensic lab "Two leaders", chapter F5-10).
// The protocol is deliberately simple (it is NOT Raft; DS302 does it properly):
//   - the leader sends a heartbeat carrying its term to the others every second;
//   - a follower that hears nothing for its timeout starts an election for term + 1;
//   - a node grants its vote to the first candidate of a newer term; a majority wins;
//   - a node that sees a newer term steps down; a heartbeat with an older term is rejected.
// Planted fault (described in the answer key): node 1 freezes for 5 s; work that queued up
// during the freeze is processed in arrival order when it resumes. Each node writes its log
// with its own wall clock, and the three wall clocks disagree.
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <string>
#include <vector>

namespace sim {

struct LogLine
{
    int trueMs;       // simulator's true time (never visible to the nodes)
    int node;         // 1..3
    int wallMs;       // the node's own wall clock
    int lamport;      // Lamport clock after the event (the nodes do NOT log it)
    std::string text;
};

enum class Type { tick, deliver, client, pauseStart, pauseEnd };
enum class Msg { heartbeat, voteRequest, vote, reject };

struct Event
{
    int t;
    std::uint64_t seq;
    Type type;
    int node;
    Msg msg = Msg::heartbeat;
    int from = 0;
    int term = 0;
    int id = 0;        // message number m1, m2, ...
    int lamport = 0;   // carried Lamport time
    std::string value; // client write
    bool operator>(const Event& o) const { return t != o.t ? t > o.t : seq > o.seq; }
};

class Cluster
{
public:
    std::vector<LogLine> run()
    {
        // initial state: node 1 is leader of term 1 (elected before the logs start)
        for (int n = 1; n <= 3; ++n) {
            term_[n] = 1;
            leader_[n] = (n == 1);
            lastHeard_[n] = 0;
            votedTerm_[n] = 1;
            push(at(500 * n, Type::tick, n));
        }
        push(at(10050, Type::pauseStart, 1));
        push(at(15050, Type::pauseEnd, 1));
        Event w1 = at(10200, Type::client, 1);
        w1.value = "x=1";
        push(w1);
        Event w2 = at(14200, Type::client, 2);
        w2.value = "x=2";
        push(w2);
        while (!q_.empty()) {
            Event e = q_.top();
            q_.pop();
            if (e.t > 17500) {
                break;
            }
            now_ = e.t;
            dispatch(e);
        }
        return log_;
    }

private:
    static int wallOffsetMs(int node)   // node 1: 3 s slow; node 3: 0.8 s fast
    {
        return node == 1 ? -3000 : node == 3 ? 800 : 0;
    }

    static Event at(int t, Type type, int node)
    {
        Event e{};
        e.t = t;
        e.type = type;
        e.node = node;
        return e;
    }

    void push(Event e)
    {
        e.seq = ++seq_;
        q_.push(e);
    }

    void write(int node, const std::string& text)
    {
        log_.push_back({now_, node, now_ + wallOffsetMs(node) + 32400000, lam_[node], text});
    }

    int delay()   // network delay 5..39 ms, from a fixed linear congruential sequence
    {
        rng_ = rng_ * 6364136223846793005ULL + 1442695040888963407ULL;
        return 5 + static_cast<int>((rng_ >> 33) % 35);
    }

    int send(int from, int to, Msg m, int term)
    {
        lam_[from] += 1;
        const int id = ++msgId_;
        Event e = at(now_ + delay(), Type::deliver, to);
        e.msg = m;
        e.from = from;
        e.term = term;
        e.id = id;
        e.lamport = lam_[from];
        push(e);
        return id;
    }

    void dispatch(const Event& e)
    {
        if (e.type == Type::pauseStart) {
            paused_ = true;
            return;
        }
        if (e.type == Type::pauseEnd) {
            paused_ = false;
            std::vector<Event> work = std::move(backlog_);
            backlog_.clear();
            for (const Event& w : work) {
                handle(w);
            }
            return;
        }
        if (e.node == 1 && paused_) {
            if (e.type != Type::tick) {
                backlog_.push_back(e);            // queued; handled after the freeze
            } else {
                push(at(e.t + 500, Type::tick, 1));
            }
            return;
        }
        handle(e);
    }

    void handle(const Event& e)
    {
        const int n = e.node;
        switch (e.type) {
        case Type::tick:
            onTick(n);
            push(at(now_ + 500, Type::tick, n));
            break;
        case Type::client:
            lam_[n] += 1;
            if (leader_[n]) {
                write(n, "client write " + e.value + ": ACCEPTED (I am leader, term "
                             + std::to_string(term_[n]) + ")");
            } else {
                write(n, "client write " + e.value + ": refused (not leader)");
            }
            break;
        case Type::deliver:
            lam_[n] = std::max(lam_[n], e.lamport) + 1;
            onMessage(n, e);
            break;
        default:
            break;
        }
    }

    void onTick(int n)
    {
        if (leader_[n]) {
            if (now_ - lastBeat_[n] >= 1000) {
                lastBeat_[n] = now_;
                std::string ids;
                for (int to = 1; to <= 3; ++to) {
                    if (to != n) {
                        ids += " m" + std::to_string(send(n, to, Msg::heartbeat, term_[n]))
                               + "->n" + std::to_string(to);
                    }
                }
                write(n, "heartbeat term " + std::to_string(term_[n]) + " sent:" + ids);
            }
            return;
        }
        const int timeout = n == 2 ? 2500 : 3200;
        if (now_ - lastHeard_[n] > timeout && !candidate_[n]) {
            term_[n] += 1;
            candidate_[n] = true;
            votes_[n] = 1;
            votedTerm_[n] = term_[n];
            lam_[n] += 1;
            std::string ids;
            for (int to = 1; to <= 3; ++to) {
                if (to != n) {
                    ids += " m" + std::to_string(send(n, to, Msg::voteRequest, term_[n]))
                           + "->n" + std::to_string(to);
                }
            }
            write(n, "no heartbeat for " + std::to_string((now_ - lastHeard_[n]) / 100 / 10.0)
                         .substr(0, 3) + " s: election for term " + std::to_string(term_[n])
                         + ", vote requests:" + ids);
        }
    }

    void stepDown(int n, int term)
    {
        term_[n] = term;
        if (leader_[n]) {
            write(n, "stepping down: saw term " + std::to_string(term));
        }
        leader_[n] = false;
        candidate_[n] = false;
    }

    void onMessage(int n, const Event& e)
    {
        const std::string mid = "m" + std::to_string(e.id);
        const std::string src = "n" + std::to_string(e.from);
        switch (e.msg) {
        case Msg::heartbeat:
            if (e.term < term_[n]) {
                const int r = send(n, e.from, Msg::reject, term_[n]);
                write(n, mid + " heartbeat from " + src + " term " + std::to_string(e.term)
                             + " REJECTED (my term " + std::to_string(term_[n]) + "), reply m"
                             + std::to_string(r));
                return;
            }
            if (e.term > term_[n] || leader_[n]) {
                stepDown(n, e.term);
            }
            candidate_[n] = false;
            lastHeard_[n] = now_;
            write(n, mid + " heartbeat from " + src + " term " + std::to_string(e.term));
            break;
        case Msg::voteRequest:
            if (e.term > term_[n]) {
                stepDown(n, e.term);
            }
            if (e.term == term_[n] && votedTerm_[n] < e.term) {
                votedTerm_[n] = e.term;
                lastHeard_[n] = now_;
                const int r = send(n, e.from, Msg::vote, e.term);
                write(n, mid + " vote request from " + src + " term " + std::to_string(e.term)
                             + ": vote granted, reply m" + std::to_string(r));
            } else {
                write(n, mid + " vote request from " + src + " term " + std::to_string(e.term)
                             + ": ignored");
            }
            break;
        case Msg::vote:
            if (candidate_[n] && e.term == term_[n]) {
                votes_[n] += 1;
                write(n, mid + " vote from " + src + " for term " + std::to_string(e.term));
                if (votes_[n] == 2) {
                    candidate_[n] = false;
                    leader_[n] = true;
                    lastBeat_[n] = -1000;
                    write(n, "became LEADER for term " + std::to_string(term_[n]));
                    onTick(n);
                }
            } else {
                write(n, mid + " vote from " + src + " for term " + std::to_string(e.term)
                             + " (already decided)");
            }
            break;
        case Msg::reject:
            write(n, mid + " rejection from " + src + " carrying term " + std::to_string(e.term));
            if (e.term > term_[n]) {
                stepDown(n, e.term);
            }
            break;
        }
    }

    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> q_;
    std::vector<Event> backlog_;
    std::vector<LogLine> log_;
    int term_[4] = {}, votedTerm_[4] = {}, votes_[4] = {}, lam_[4] = {};
    int lastHeard_[4] = {}, lastBeat_[4] = {-1000, -1000, -1000, -1000};
    bool leader_[4] = {}, candidate_[4] = {};
    bool paused_ = false;
    int now_ = 0, msgId_ = 0;
    std::uint64_t seq_ = 0, rng_ = 12345;
};

inline std::string clock(int ms)   // hh:mm:ss.mmm
{
    char buf[32];
    std::snprintf(buf, sizeof buf, "%02d:%02d:%02d.%03d", ms / 3600000, ms / 60000 % 60,
                  ms / 1000 % 60, ms % 1000);
    return buf;
}

}  // namespace sim
