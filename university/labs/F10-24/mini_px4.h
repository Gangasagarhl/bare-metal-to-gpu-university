// mini_px4.h - this course's miniature flight-stack runtime, used to practise the
// shape of a PX4-style module without PX4: a simulated clock, uorb_lite topics,
// a parameter store that announces changes on a topic, and a scheduler that runs
// work items on fixed intervals. Our own code; not PX4's API.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "uorb_lite.h"

namespace mini_px4 {

struct Attitude
{
    std::uint64_t timestampUs;
    float rollDeg;
    float pitchDeg;
};

struct TiltStatus
{
    std::uint64_t timestampUs;
    float tiltDeg;
    float maxTiltDeg;
    bool exceeded;
    std::uint32_t exceedCount;
};

struct ParameterUpdate
{
    std::uint64_t timestampUs;
    std::uint32_t changeCount;
};

class System
{
public:
    std::uint64_t nowUs = 0;
    uorb_lite::Topic<Attitude, 1> attitude{"vehicle_attitude_model"};
    uorb_lite::Topic<TiltStatus, 1> tiltStatus{"tilt_status"};
    uorb_lite::Topic<ParameterUpdate, 1> parameterUpdate{"parameter_update_model"};

    void defineParam(const std::string& name, float value) { params_[name] = value; }

    bool setParam(const std::string& name, float value)
    {
        auto it = params_.find(name);
        if (it == params_.end()) {
            return false;
        }
        it->second = value;
        parameterUpdate.publish(ParameterUpdate{nowUs, ++changes_});   // tell every module
        return true;
    }

    bool param(const std::string& name, float& out) const
    {
        auto it = params_.find(name);
        if (it == params_.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

    int scheduleOnInterval(std::string owner, std::uint64_t intervalUs, std::function<void()> fn)
    {
        work_.push_back(Work{std::move(owner), intervalUs, nowUs + intervalUs, std::move(fn), true});
        return static_cast<int>(work_.size()) - 1;
    }

    void cancel(int id)
    {
        if (id >= 0 && static_cast<std::size_t>(id) < work_.size()) {
            work_[static_cast<std::size_t>(id)].active = false;
        }
    }

    // Advances simulated time in 1 ms steps. The "simulator" publishes the attitude of
    // a gentle rocking motion every 4 ms; due work items run after it.
    void runFor(double seconds)
    {
        const std::uint64_t end = nowUs + static_cast<std::uint64_t>(seconds * 1e6 + 0.5);
        while (nowUs < end) {
            nowUs += 1000;
            if (nowUs % 4000 == 0) {
                const double t = static_cast<double>(nowUs) * 1e-6;
                const double pi = 3.14159265358979323846;
                attitude.publish(Attitude{nowUs, static_cast<float>(25.0 * std::sin(pi * t)),
                                          static_cast<float>(12.0 * std::sin(pi * t + 1.0))});
            }
            for (auto& w : work_) {
                if (w.active && nowUs >= w.nextUs) {
                    w.nextUs += w.intervalUs;
                    w.fn();
                }
            }
        }
    }

    void printTopics() const
    {
        std::printf("%-24s %4s %10s\n", "topic", "inst", "published");
        std::printf("%-24s %4d %10llu\n", attitude.name().c_str(), attitude.instance(),
                    static_cast<unsigned long long>(attitude.generation()));
        std::printf("%-24s %4d %10llu\n", tiltStatus.name().c_str(), tiltStatus.instance(),
                    static_cast<unsigned long long>(tiltStatus.generation()));
        std::printf("%-24s %4d %10llu\n", parameterUpdate.name().c_str(), parameterUpdate.instance(),
                    static_cast<unsigned long long>(parameterUpdate.generation()));
    }

private:
    struct Work
    {
        std::string owner;
        std::uint64_t intervalUs;
        std::uint64_t nextUs;
        std::function<void()> fn;
        bool active;
    };
    std::map<std::string, float> params_;
    std::vector<Work> work_;
    std::uint32_t changes_ = 0;
};

} // namespace mini_px4
