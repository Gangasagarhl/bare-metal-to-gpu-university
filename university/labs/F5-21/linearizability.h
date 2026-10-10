// linearizability.h - is a history of puts and gets on one register linearizable?
// Search in the style of Wing and Gong: repeatedly pick an operation that may
// take effect next (no unpicked operation finished before it started), apply it
// to a model register, and backtrack on a contradiction. Visited (set, value)
// pairs are remembered so that no state is explored twice.
#pragma once
#include <cstdint>
#include <cstdio>
#include <set>
#include <utility>
#include <vector>

#include "cluster.h"

namespace lin {

using raft::HistoryOp;
using raft::Time;

class Checker
{
public:
    // Gets without a reply tell us nothing and are dropped. Puts without a reply
    // may have taken effect (at any time after their call) or not at all; one
    // whose value no get ever returned can always be treated as "not at all"
    // (every put writes a different value), so it is dropped too.
    explicit Checker(const std::vector<HistoryOp>& history)
    {
        std::set<int> seen;
        for (const HistoryOp& h : history) {
            if (!h.isPut && h.response >= 0) {
                seen.insert(h.value);
            }
        }
        for (const HistoryOp& h : history) {
            if (h.response >= 0 || (h.isPut && seen.count(h.value))) {
                ops_.push_back(h);
            }
        }
    }

    std::size_t size() const { return ops_.size(); }
    bool tooBig() const { return ops_.size() > 63; }

    // true if some order exists; `order` receives it (indices into ops())
    bool check(std::vector<int>& order)
    {
        order.clear();
        seen_.clear();
        return search(0, 0, order);
    }

    const std::vector<HistoryOp>& ops() const { return ops_; }

private:
    static constexpr Time kNever = INT64_MAX;
    static Time resp(const HistoryOp& h) { return h.response < 0 ? kNever : h.response; }

    bool search(std::uint64_t done, int value, std::vector<int>& order)
    {
        const int n = static_cast<int>(ops_.size());
        bool finished = true;
        Time firstEnd = kNever;  // earliest reply among operations not yet placed
        for (int i = 0; i < n; ++i) {
            if (!(done >> i & 1U)) {
                firstEnd = std::min(firstEnd, resp(ops_[i]));
                finished = finished && ops_[i].response < 0;  // only unknown puts left?
            }
        }
        if (finished) {
            return true;
        }
        if (!seen_.insert({done, value}).second) {
            return false;
        }
        for (int i = 0; i < n; ++i) {
            const HistoryOp& h = ops_[i];
            if ((done >> i & 1U) || h.invoke > firstEnd) {
                continue;  // already placed, or some other operation ended before it began
            }
            if (!h.isPut && h.value != value) {
                continue;  // a get must return the current value
            }
            order.push_back(i);
            if (search(done | (std::uint64_t{1} << i), h.isPut ? h.value : value, order)) {
                return true;
            }
            order.pop_back();
        }
        return false;
    }

    std::vector<HistoryOp> ops_;
    std::set<std::pair<std::uint64_t, int>> seen_;
};

inline void printHistory(const std::vector<HistoryOp>& ops)
{
    std::printf("  process  operation      called   answered\n");
    for (const HistoryOp& h : ops) {
        char what[32];
        std::snprintf(what, sizeof what, h.isPut ? "put x=%d" : "get x -> %d", h.value);
        if (!h.isPut && h.response < 0) {
            std::snprintf(what, sizeof what, "get x -> ?");
        }
        if (h.response < 0) {
            std::printf("  %7d  %-13s %6lld ms   no reply\n", h.process, what,
                        static_cast<long long>(h.invoke));
        } else {
            std::printf("  %7d  %-13s %6lld ms  %6lld ms\n", h.process, what,
                        static_cast<long long>(h.invoke), static_cast<long long>(h.response));
        }
    }
}

} // namespace lin
