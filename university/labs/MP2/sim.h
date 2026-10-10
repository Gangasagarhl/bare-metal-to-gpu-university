// sim.h - deterministic network simulator for the DS302 labs (F5-15 to F5-21).
// Time is simulated milliseconds; nothing here reads the wall clock, so the same
// seed always produces the same run, on every machine, byte for byte.
#pragma once
#include <algorithm>
#include <cstdint>
#include <queue>
#include <set>
#include <utility>
#include <vector>

namespace sim {

using Time = std::int64_t;

// splitmix64: a tiny random generator whose sequence depends only on the seed.
class Rng
{
public:
    explicit Rng(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next()
    {
        std::uint64_t z = (state_ += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

    // uniform enough for a simulator; inclusive on both ends
    std::int64_t range(std::int64_t lo, std::int64_t hi)
    {
        const auto span = static_cast<std::uint64_t>(hi - lo + 1);
        return lo + static_cast<std::int64_t>(next() % span);
    }

    bool percent(int p) { return range(0, 99) < p; }

private:
    std::uint64_t state_;
};

struct NetOptions
{
    Time minDelay = 2;       // every message takes between minDelay and maxDelay ms
    Time maxDelay = 10;
    int dropPercent = 0;     // chance that a message is lost
    int duplicatePercent = 0; // chance that a message is delivered twice
};

// Messages in flight, delivered in order of arrival time (ties: order of sending).
// Random delays mean two messages between the same pair can overtake each other.
template <typename Msg>
class Network
{
public:
    Network(NetOptions opt, Rng& rng) : opt_(opt), rng_(rng) {}

    void send(Time now, const Msg& m)
    {
        if (cut(m.from, m.to) || rng_.percent(opt_.dropPercent)) {
            ++dropped_;
            return;
        }
        push(now + rng_.range(opt_.minDelay, opt_.maxDelay), m);
        if (rng_.percent(opt_.duplicatePercent)) {
            push(now + rng_.range(opt_.minDelay, opt_.maxDelay), m);
        }
    }

    // Everything due at or before `now`. A link cut while a message was in
    // flight also loses that message.
    std::vector<Msg> due(Time now)
    {
        std::vector<Msg> out;
        while (!q_.empty() && q_.top().at <= now) {
            const Msg m = q_.top().msg;
            q_.pop();
            if (cut(m.from, m.to)) {
                ++dropped_;
            } else {
                out.push_back(m);
            }
        }
        return out;
    }

    void block(int a, int b) { cut_.insert(key(a, b)); }
    void unblock(int a, int b) { cut_.erase(key(a, b)); }
    void healAll() { cut_.clear(); }
    bool cut(int a, int b) const { return cut_.count(key(a, b)) != 0; }
    NetOptions& options() { return opt_; }
    long dropped() const { return dropped_; }

private:
    struct InFlight
    {
        Time at;
        long seq;
        Msg msg;
    };
    struct Later
    {
        bool operator()(const InFlight& a, const InFlight& b) const
        {
            return a.at != b.at ? a.at > b.at : a.seq > b.seq;
        }
    };

    static std::pair<int, int> key(int a, int b) { return {std::min(a, b), std::max(a, b)}; }
    void push(Time at, const Msg& m) { q_.push(InFlight{at, seq_++, m}); }

    NetOptions opt_;
    Rng& rng_;
    std::priority_queue<InFlight, std::vector<InFlight>, Later> q_;
    std::set<std::pair<int, int>> cut_;
    long seq_ = 0;
    long dropped_ = 0;
};

} // namespace sim
