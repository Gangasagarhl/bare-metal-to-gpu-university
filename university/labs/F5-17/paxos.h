// paxos.h - single-decree Paxos ("Paxos Made Simple", sections 2.2-2.3) for
// three acceptors and two proposers, on a deterministic simulated network.
#pragma once
#include <cstdarg>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "sim.h"

namespace paxos {

using sim::Time;

enum class Kind { Prepare, Promise, Accept, Accepted };

struct Msg
{
    Kind kind{};
    int from = 0;
    int to = 0;
    int ballot = 0;          // the proposal number n
    int acceptedBallot = 0;  // Promise: highest ballot this acceptor accepted (0: none)
    std::string value;       // Accept/Accepted: the value; Promise: the accepted value
    Time at = 0;             // delivery time
    long seq = 0;
};

// What an acceptor must keep on stable storage.
struct AcceptorState
{
    int promised = 0;        // highest ballot it promised (in a Promise or an accept)
    int acceptedBallot = 0;  // ballot of the proposal it accepted last (0: none)
    std::string acceptedValue;
};

class World
{
public:
    // bugCompareWithAccepted: the acceptor checks an Accept against the last
    // ballot it ACCEPTED instead of the last ballot it PROMISED (forensic lab).
    explicit World(std::uint64_t seed, bool bugCompareWithAccepted = false)
        : rng_(seed), bug_(bugCompareWithAccepted)
    {
        for (int a : kAcceptors) {
            acceptors_[a] = AcceptorState{};
        }
    }

    static constexpr int kAcceptors[3] = {1, 2, 3};
    bool verbose = true;

    // per-link delay (ms) for scripted scenarios; default 5 ms, or random if set
    void setDelay(int from, int to, Time d) { delay_[{from, to}] = d; }
    void randomDelays(Time lo, Time hi) { randLo_ = lo; randHi_ = hi; }

    // A proposer starts (or restarts) phase 1 with a ballot above any it used.
    void propose(int proposer, const std::string& value)
    {
        Proposer& p = proposers_[proposer];
        p.value = value;
        p.round += 1;
        p.ballot = p.round * 10 + proposer % 10;  // unique: proposers end in different digits
        p.promises.clear();
        p.accepts.clear();
        p.highestSeen = 0;
        p.adopted = value;
        p.phase2 = false;
        p.done = false;
        p.retryAt = now_ + retryFixed_ + (retryRandom_ ? rng_.range(0, retryRandom_) : 0);
        say("P%d: prepare(%d) for value %s", proposer % 10, p.ballot, value.c_str());
        for (int a : kAcceptors) {
            send(Msg{Kind::Prepare, proposer, a, p.ballot, 0, "", 0, 0});
        }
    }

    void crashAcceptor(int a) { down_.insert(a); say("a%d: *** CRASH ***", a); }
    void restartAcceptor(int a) { down_.erase(a); say("a%d: restarted with its state from disk", a); }

    void runUntil(Time end)
    {
        while (now_ < end) {
            ++now_;
            std::vector<Msg> due;
            while (!queue_.empty() && queue_.begin()->first.first <= now_) {
                due.push_back(queue_.begin()->second);
                queue_.erase(queue_.begin());
            }
            for (const Msg& m : due) {
                deliver(m);
            }
            for (auto& [id, p] : proposers_) {
                if (!p.done && now_ >= p.retryAt) {
                    say("P%d: no majority for ballot %d in time; tries again", id % 10, p.ballot);
                    propose(id, p.value);
                }
            }
        }
    }

    // A proposer that has not learned a chosen value this long after its prepare
    // starts again with a higher ballot: a fixed timeout, plus an optional random part.
    void setRetry(Time fixed, Time randomExtra) { retryFixed_ = fixed; retryRandom_ = randomExtra; }

    Time now() const { return now_; }
    bool someProposerKnows() const
    {
        for (const auto& [id, p] : proposers_) {
            if (p.done) {
                return true;
            }
        }
        return false;
    }
    const std::map<int, std::string>& chosen() const { return chosen_; }
    int ballotsTried() const
    {
        int n = 0;
        for (const auto& [id, p] : proposers_) {
            n += p.round;
        }
        return n;
    }

    bool report() const  // true if safe
    {
        if (chosen_.empty()) {
            std::printf("chosen: nothing yet (at %lld ms)\n", static_cast<long long>(now_));
        }
        for (const auto& [ballot, value] : chosen_) {
            std::printf("chosen: value %s in ballot %d\n", value.c_str(), ballot);
        }
        std::set<std::string> values;
        for (const auto& [ballot, value] : chosen_) {
            values.insert(value);
        }
        std::printf("safety: %s\n", values.size() <= 1 ? "ok, at most one value chosen"
                                                        : "VIOLATED, two different values chosen");
        return values.size() <= 1;
    }

private:
    struct Proposer
    {
        std::string value;   // the value this proposer would like
        std::string adopted; // the value it will actually propose in phase 2
        int round = 0;
        int ballot = 0;
        int highestSeen = 0; // highest acceptedBallot among promises
        std::set<int> promises;
        std::set<int> accepts;
        bool phase2 = false;
        bool done = false;   // a majority accepted my ballot: I know the value is chosen
        Time retryAt = 0;
    };

    void say(const char* fmt, ...) __attribute__((format(printf, 2, 3)))
    {
        if (!verbose) {
            return;
        }
        va_list ap;
        va_start(ap, fmt);
        std::printf("%5lld ms  ", static_cast<long long>(now_));
        std::vprintf(fmt, ap);
        std::printf("\n");
        va_end(ap);
    }

    void send(Msg m)
    {
        Time d = 5;
        const auto it = delay_.find({m.from, m.to});
        if (it != delay_.end()) {
            d = it->second;
        } else if (randHi_ > 0) {
            d = rng_.range(randLo_, randHi_);
        }
        m.at = now_ + d;
        m.seq = seq_++;
        queue_.emplace(std::make_pair(m.at, m.seq), m);
    }

    void deliver(const Msg& m)
    {
        if (down_.count(m.to) || down_.count(m.from)) {
            return;
        }
        switch (m.kind) {
        case Kind::Prepare: onPrepare(m); break;
        case Kind::Accept: onAccept(m); break;
        case Kind::Promise: onPromise(m); break;
        case Kind::Accepted: onAccepted(m); break;
        }
    }

    // Phase 1b: promise never to accept a ballot below n, and report what I accepted.
    void onPrepare(const Msg& m)
    {
        AcceptorState& a = acceptors_[m.to];
        if (m.ballot > a.promised) {
            a.promised = m.ballot;  // (written to stable storage before replying)
            say("a%d: promises ballot %d%s", m.to, m.ballot,
                a.acceptedBallot ? (", reports accepted (" + std::to_string(a.acceptedBallot) +
                                    ", " + a.acceptedValue + ")").c_str()
                                 : ", has accepted nothing");
            send(Msg{Kind::Promise, m.to, m.from, m.ballot, a.acceptedBallot, a.acceptedValue, 0, 0});
        } else {
            say("a%d: ignores prepare(%d), already promised %d", m.to, m.ballot, a.promised);
        }
    }

    // Phase 2b: accept unless that would break a promise.
    void onAccept(const Msg& m)
    {
        AcceptorState& a = acceptors_[m.to];
        const int limit = bug_ ? a.acceptedBallot : a.promised;  // the forensic bug
        if (m.ballot >= limit) {
            a.promised = m.ballot;
            a.acceptedBallot = m.ballot;
            a.acceptedValue = m.value;
            say("a%d: accepts (%d, %s)", m.to, m.ballot, m.value.c_str());
            send(Msg{Kind::Accepted, m.to, m.from, m.ballot, 0, m.value, 0, 0});
            learn(m.to, m.ballot, m.value);
        } else {
            say("a%d: ignores accept(%d, %s), promised %d", m.to, m.ballot, m.value.c_str(),
                a.promised);
        }
    }

    void onPromise(const Msg& m)
    {
        Proposer& p = proposers_[m.to];
        if (m.ballot != p.ballot || p.phase2) {
            return;  // an old reply
        }
        p.promises.insert(m.from);
        if (m.acceptedBallot > p.highestSeen) {
            p.highestSeen = m.acceptedBallot;
            p.adopted = m.value;  // must propose the value of the highest accepted ballot
        }
        if (p.promises.size() == 2) {  // a majority of three
            p.phase2 = true;
            if (p.adopted != p.value) {
                say("P%d: majority promised; must adopt value %s (accepted in ballot %d)",
                    m.to % 10, p.adopted.c_str(), p.highestSeen);
            } else {
                say("P%d: majority promised; free to propose its own value %s", m.to % 10,
                    p.value.c_str());
            }
            for (int a : kAcceptors) {
                send(Msg{Kind::Accept, m.to, a, p.ballot, 0, p.adopted, 0, 0});
            }
        }
    }

    void onAccepted(const Msg& m)
    {
        Proposer& p = proposers_[m.to];
        if (m.ballot == p.ballot && !p.done) {
            p.accepts.insert(m.from);
            if (p.accepts.size() == 2) {
                p.done = true;
                say("P%d: majority accepted ballot %d: knows that %s is chosen", m.to % 10,
                    p.ballot, m.value.c_str());
            }
        }
    }

    // The learner's view: a value is chosen when a majority accepted the same ballot.
    void learn(int acceptor, int ballot, const std::string& value)
    {
        acceptedBy_[ballot].insert(acceptor);
        if (acceptedBy_[ballot].size() == 2 && !chosen_.count(ballot)) {
            chosen_[ballot] = value;
            say("    >>> value %s is CHOSEN (ballot %d, majority of acceptors)", value.c_str(),
                ballot);
        }
    }

    sim::Rng rng_;
    bool bug_;
    Time now_ = 0;
    long seq_ = 0;
    std::map<std::pair<Time, long>, Msg> queue_;
    std::map<std::pair<int, int>, Time> delay_;
    Time randLo_ = 0;
    Time randHi_ = 0;
    Time retryFixed_ = 50;
    Time retryRandom_ = 0;
    std::map<int, AcceptorState> acceptors_;
    std::map<int, Proposer> proposers_;
    std::set<int> down_;
    std::map<int, std::set<int>> acceptedBy_;
    std::map<int, std::string> chosen_;
};

} // namespace paxos
