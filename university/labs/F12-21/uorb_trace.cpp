// uorb_trace.cpp - follow ONE sensor sample from a driver to an estimator and on to a ROS 2 node.
// A discrete-event model written for this course (it is NOT PX4 or ROS 2 code): modules publish
// topics on a notice board; every message carries a trace of the hops it passed and when.
// Configuration on stdin: "sensors triggered" or "sensors polled <interval_us>",
// then "bridge <period_us>" and "run <duration_us>".
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Hop
{
    std::string module;
    long at_us;
};

struct Msg
{
    std::uint64_t gen = 0;        // generation in the topic it was last published on: 1, 2, ...
    std::uint64_t accel_gen = 0;  // generation of the accelerometer sample it carries
    long sample_us = 0;           // when the accelerometer sample was taken
    std::vector<Hop> hops;
};

struct Topic
{
    std::string name;
    Msg last;
    std::vector<std::function<void(long)>> onPublish;  // modules triggered by a publication
    void publish(Msg m, long now)
    {
        m.gen = last.gen + 1;
        last = m;
        for (auto& f : onPublish) f(now);
    }
};

struct Reader  // one subscription: remembers the last generation it has seen
{
    std::uint64_t seen = 0;
    long missed = 0;
    bool take(const Topic& t, Msg& out)
    {
        if (t.last.gen == seen) return false;  // nothing new
        missed += long(t.last.gen - seen - 1);  // generations overwritten before we looked
        seen = t.last.gen;
        out = t.last;
        return true;
    }
};

std::multimap<long, std::function<void(long)>> events;  // time -> action
void at(long t, std::function<void(long)> f) { events.insert({t, std::move(f)}); }

int main()
{
    std::string word, mode;
    long poll_us = 0, bridge_us = 0, run_us = 0;
    std::cin >> word >> mode;
    if (mode == "polled") std::cin >> poll_us;
    std::cin >> word >> bridge_us >> word >> run_us;
    std::cout << "config: sensors " << mode << (poll_us ? " every " + std::to_string(poll_us)
              + " us" : "") << "; bridge every " << bridge_us << " us; run " << run_us << " us\n";

    Topic accel{"sensor_accel_model", {}, {}}, imu{"vehicle_imu_model", {}, {}};
    Topic est{"estimator_state_model", {}, {}};
    Reader sensorsR, estR, bridgeR;
    std::vector<Msg> delivered;  // what the ROS 2 listener received
    long estMin = 1L << 40, estMax = 0;  // age of the sample when the estimator finished

    for (long t = 0; t < run_us; t += 1000) {  // accel_driver: one sample per 1000 us
        at(t + 50, [&accel, t](long now) {
            accel.publish(Msg{0, accel.last.gen + 1, t, {{"accel_driver", now}}}, now);
        });
    }
    auto sensorsRun = [&](long now) {  // sensors module: accel -> vehicle_imu
        Msg m;
        if (!sensorsR.take(accel, m)) return;
        m.hops.push_back({"sensors", now});
        imu.publish(m, now);
    };
    if (mode == "triggered") {
        accel.onPublish.push_back([&](long now) { at(now + 30, sensorsRun); });
    } else {
        for (long t = 500; t < run_us; t += poll_us) at(t, sensorsRun);
    }
    imu.onPublish.push_back([&](long now) {  // estimator: triggered by vehicle_imu, 200 us of work
        at(now + 200, [&](long done) {
            Msg m;
            if (!estR.take(imu, m)) return;
            m.hops.push_back({"estimator", done});
            estMin = std::min(estMin, done - m.sample_us);
            estMax = std::max(estMax, done - m.sample_us);
            est.publish(m, done);
        });
    });
    for (long t = 300; t < run_us; t += bridge_us) {  // bridge to ROS 2, then a listener callback
        at(t, [&](long now) {
            Msg m;
            if (!bridgeR.take(est, m)) return;
            m.hops.push_back({"dds_bridge", now});
            at(now + 150, [&delivered, m](long cb) mutable {
                m.hops.push_back({"ros2_listener", cb});
                delivered.push_back(m);
            });
        });
    }
    while (!events.empty()) {
        auto it = events.begin();
        auto [t, f] = *it;
        events.erase(it);
        f(t);
    }

    std::map<std::string, std::pair<long, long>> added;  // module -> min, max latency it adds
    for (const Msg& m : delivered) {
        long prev = m.sample_us;
        std::cout << "sample t=" << m.sample_us << " us (accel gen " << m.accel_gen << "):";
        for (const Hop& h : m.hops) {
            std::cout << " " << h.module << " +" << (h.at_us - prev);
            auto [it, fresh] = added.insert({h.module, {h.at_us - prev, h.at_us - prev}});
            if (!fresh) {
                it->second.first = std::min(it->second.first, h.at_us - prev);
                it->second.second = std::max(it->second.second, h.at_us - prev);
            }
            prev = h.at_us;
        }
        std::cout << "  -> age at listener " << (prev - m.sample_us) << " us\n";
    }
    std::cout << "latency added per hop (min..max over delivered samples):\n";
    for (const char* m : {"accel_driver", "sensors", "estimator", "dds_bridge", "ros2_listener"}) {
        std::cout << "  " << m << ": " << added[m].first << ".." << added[m].second << " us\n";
    }
    std::cout << "age of the sample when the estimator finished: " << estMin << ".." << estMax
              << " us\n";
    std::cout << "accel samples published: " << accel.last.gen << "; used by the estimator: "
              << est.last.gen << "; overwritten before sensors read them: " << sensorsR.missed
              << "\nestimator outputs not forwarded (the bridge runs slower on purpose): "
              << bridgeR.missed << "\n";
    return 0;
}
