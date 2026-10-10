// wq_model.h - a deterministic model of work queues on one processor.
// Each work queue is one thread with a fixed priority. Work items are queued on a
// work queue and run there one after another, each to completion. Threads of higher
// priority preempt lower ones. An item that is already queued is not queued twice:
// a second request is merged (and counted). Time unit: 1 microsecond.
#pragma once

#include <algorithm>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace wq_model {

struct WorkQueue
{
    std::string name;
    int priority;
};

struct Item
{
    std::string name;
    int wq;           // index into the work-queue table
    int period;       // us; 0 = not periodic
    int cost;         // us of processor time per run
    int triggeredBy;  // index of the item whose completion queues this one; -1 = none
};

struct Stats
{
    int runs = 0;
    int merged = 0;      // requests that arrived while the item was still queued or running
    int worstStart = 0;  // queued -> started
    int worstResponse = 0;
    long busy = 0;
};

struct Config
{
    std::string title;
    std::vector<WorkQueue> queues;
    std::vector<Item> items;
};

struct Running
{
    int item = -1;
    int left = 0;
};

inline void simulate(const Config& c, int durationUs, int traceItem)
{
    const std::size_t nq = c.queues.size();
    const std::size_t ni = c.items.size();
    std::vector<std::deque<int>> fifo(nq);
    std::vector<Running> running(nq);
    std::vector<int> queuedAt(ni, -1);   // -1 = idle; otherwise the time it was queued
    std::vector<Stats> st(ni);
    std::vector<long> wqBusy(nq, 0);
    int worstTraceStart = -1;
    int worstTraceQueued = 0;
    std::string timeline(static_cast<std::size_t>(durationUs), '.');

    auto request = [&](int i, int t) {
        if (queuedAt[static_cast<std::size_t>(i)] >= 0) {
            ++st[static_cast<std::size_t>(i)].merged;
            return;
        }
        queuedAt[static_cast<std::size_t>(i)] = t;
        fifo[static_cast<std::size_t>(c.items[static_cast<std::size_t>(i)].wq)].push_back(i);
    };

    for (int t = 0; t < durationUs; ++t) {
        for (std::size_t i = 0; i < ni; ++i) {
            const Item& it = c.items[i];
            if (it.period > 0 && t % it.period == 0) {
                request(static_cast<int>(i), t);
            }
        }
        // The highest-priority work-queue thread that has work runs for 1 us.
        int best = -1;
        for (std::size_t q = 0; q < nq; ++q) {
            if (running[q].item < 0 && fifo[q].empty()) {
                continue;
            }
            if (best < 0 || c.queues[q].priority > c.queues[static_cast<std::size_t>(best)].priority) {
                best = static_cast<int>(q);
            }
        }
        if (best < 0) {
            continue;
        }
        Running& r = running[static_cast<std::size_t>(best)];
        if (r.item < 0) {
            r.item = fifo[static_cast<std::size_t>(best)].front();
            fifo[static_cast<std::size_t>(best)].pop_front();
            r.left = c.items[static_cast<std::size_t>(r.item)].cost;
            Stats& s = st[static_cast<std::size_t>(r.item)];
            const int wait = t - queuedAt[static_cast<std::size_t>(r.item)];
            if (wait > s.worstStart) {
                s.worstStart = wait;
                if (r.item == traceItem) {
                    worstTraceStart = t;
                    worstTraceQueued = queuedAt[static_cast<std::size_t>(r.item)];
                }
            }
        }
        const char mark = c.items[static_cast<std::size_t>(r.item)].name[0];
        timeline[static_cast<std::size_t>(t)] = mark;
        ++st[static_cast<std::size_t>(r.item)].busy;
        ++wqBusy[static_cast<std::size_t>(best)];
        if (--r.left == 0) {
            const int done = r.item;
            Stats& s = st[static_cast<std::size_t>(done)];
            ++s.runs;
            s.worstResponse = std::max(s.worstResponse, t + 1 - queuedAt[static_cast<std::size_t>(done)]);
            queuedAt[static_cast<std::size_t>(done)] = -1;
            r.item = -1;
            for (std::size_t j = 0; j < ni; ++j) {
                if (c.items[j].triggeredBy == done) {
                    request(static_cast<int>(j), t + 1);
                }
            }
        }
    }

    std::printf("== %s (simulated %d ms) ==\n", c.title.c_str(), durationUs / 1000);
    std::printf("%-14s %-10s %4s %-16s %5s %5s %6s %10s %10s\n", "item", "queue", "prio", "period",
                "cost", "runs", "merged", "worst wait", "worst resp");
    for (std::size_t i = 0; i < ni; ++i) {
        const Item& it = c.items[i];
        const WorkQueue& q = c.queues[static_cast<std::size_t>(it.wq)];
        const std::string per = it.period > 0 ? std::to_string(it.period)
                                              : "after " + c.items[static_cast<std::size_t>(it.triggeredBy)].name;
        std::printf("%-14s %-10s %4d %-16s %5d %5d %6d %10d %10d\n", it.name.c_str(), q.name.c_str(),
                    q.priority, per.c_str(), it.cost, st[i].runs, st[i].merged, st[i].worstStart,
                    st[i].worstResponse);
    }
    std::printf("processor use per work-queue thread:");
    for (std::size_t q = 0; q < nq; ++q) {
        std::printf(" %s %.1f %%", c.queues[q].name.c_str(), 100.0 * static_cast<double>(wqBusy[q]) / durationUs);
    }
    std::printf("\n");
    if (worstTraceStart >= 0) {
        // One character per 50 us around the worst wait of the traced item.
        const int from = std::max(0, worstTraceQueued - 500);
        const int to = std::min(durationUs, worstTraceStart + 500);
        std::printf("timeline %d..%d us, one character per 50 us (first letter of the running item, '.' idle):\n",
                    from, to);
        std::string line;
        for (int t = from; t < to; t += 50) {
            line += timeline[static_cast<std::size_t>(t)];
        }
        std::printf("  %s\n", line.c_str());
        std::printf("  %s queued at %d us, started at %d us\n",
                    c.items[static_cast<std::size_t>(traceItem)].name.c_str(), worstTraceQueued, worstTraceStart);
    }
    std::printf("\n");
}

// Work-queue threads of this model; the names are this course's, not PX4's.
inline std::vector<WorkQueue> queues()
{
    return {{"wq_sensor", 10}, {"wq_rate", 9}, {"wq_ctrl", 7}, {"wq_lowprio", 2}};
}

// Configuration A: each item on the work queue that matches its urgency.
inline Config configA()
{
    return Config{"A: each item on the queue that matches its urgency", queues(), {
        {"imu_driver", 0, 1000, 60, -1},
        {"rate_ctrl", 1, 0, 150, 0},           // runs after every new IMU sample
        {"attitude_ctrl", 2, 4000, 300, -1},
        {"position_ctrl", 2, 20000, 600, -1},
        {"battery", 3, 100000, 400, -1},
        {"mag_cal_step", 3, 50000, 2500, -1},
    }};
}

} // namespace wq_model
