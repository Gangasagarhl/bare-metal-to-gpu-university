// bag_tool.cpp - the university's model of recording, inspecting, replaying and tracing (F9-43).
// Not a ROS 2 bag: an in-memory recording of time-stamped messages from a modelled 10 s drive,
// a summary like a bag's "info", a text plot, a replay at a chosen rate to a node that
// integrates speed, and a small trace analysis of callback latency.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Msg { std::string topic; long stampUs; double value; };   // header stamp, payload
using Bag = std::vector<Msg>;

double speedAt(long us)                    // the modelled drive: 0.5 m/s, then 0.25 m/s, then stop
{
    if (us < 4000000) return 0.5;
    if (us < 8000000) return 0.25;
    return 0.0;
}

Bag record()
{
    Bag bag;
    for (long us = 0; us < 10000000; us += 2000) {           // 2 ms steps
        if (us % 100000 == 0) bag.push_back({"/wheel_speed", us, speedAt(us)});   // 10 Hz
        if (us % 20000 == 0) bag.push_back({"/imu", us, 0.0});                     // 50 Hz
    }
    return bag;
}

void info(const Bag& bag)
{
    std::map<std::string, int> count;
    for (const auto& m : bag) count[m.topic]++;
    std::printf("bag: duration %.3f s, start %.3f s, end %.3f s, messages %zu\n",
                (bag.back().stampUs - bag.front().stampUs) / 1e6, bag.front().stampUs / 1e6, bag.back().stampUs / 1e6, bag.size());
    for (const auto& [t, n] : count) std::printf("  topic %-13s count %d\n", t.c_str(), n);
}

void plot(const Bag& bag)
{
    std::printf("plot /wheel_speed (one column per 0.2 s, '#' = 0.05 m/s)\n");
    for (int level = 10; level >= 1; --level) {
        std::printf("  %4.2f |", level * 0.05);
        for (const auto& m : bag)
            if (m.topic == "/wheel_speed" && m.stampUs % 200000 == 0) std::printf("%c", m.value >= level * 0.05 - 1e-9 ? '#' : ' ');
        std::printf("\n");
    }
    std::printf("       +--------------------------------------------------> t (0..10 s)\n");
}

// Replay at 'rate': message i is delivered at wall time (stamp - start) / rate.
// The odometry node integrates distance with dt taken either from header stamps or from arrival time.
void replay(const Bag& bag, double rate, bool useStamps, bool debug)
{
    double dist = 0.0, prevV = 0.0;
    long prevT = -1;
    int shown = 0;
    for (const auto& m : bag) {
        if (m.topic != "/wheel_speed") continue;
        long arrivalUs = static_cast<long>((m.stampUs - bag.front().stampUs) / rate);
        long t = useStamps ? m.stampUs : arrivalUs;
        if (prevT >= 0) {
            double dt = (t - prevT) / 1e6;
            dist += prevV * dt;
            if (debug && shown < 3) { std::printf("    msg stamp %.3f s arrived %.3f s -> dt %.3f s\n", m.stampUs / 1e6, arrivalUs / 1e6, dt); ++shown; }
        }
        prevT = t; prevV = m.value;
    }
    std::printf("replay rate %.1f, node integrates with %s: distance %.3f m\n", rate,
                useStamps ? "header stamps" : "arrival time", dist);
}

// Trace: one executor thread; each callback costs 'costUs'; latency = callback start - arrival.
void trace(const Bag& bag, long costUs)
{
    long freeAt = 0;
    std::vector<long> lat;
    for (const auto& m : bag) {
        long start = std::max(m.stampUs, freeAt);
        lat.push_back(start - m.stampUs);
        freeAt = start + costUs;
    }
    std::sort(lat.begin(), lat.end());
    auto pct = [&](double p) { return lat[static_cast<std::size_t>(p * (lat.size() - 1))]; };
    std::printf("trace: %zu callbacks, cost %ld us each: latency median %ld us, p99 %ld us, max %ld us\n",
                lat.size(), costUs, pct(0.5), pct(0.99), lat.back());
}

int main()
{
    Bag bag = record();
    std::string cmd;
    while (std::cin >> cmd) {
        if (cmd == "info") info(bag);
        else if (cmd == "plot") plot(bag);
        else if (cmd == "replay") {
            double rate; std::string clock, dbg; std::cin >> rate >> clock >> dbg;
            replay(bag, rate, clock == "stamps", dbg == "debug");
        }
        else if (cmd == "trace") { long c; std::cin >> c; trace(bag, c); }
    }
    return 0;
}
