// tilt_guard_v0.h - the first version of the module, as it was reviewed in the
// F10-24 forensic lab. Compare it with tilt_guard.h.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "mini_px4.h"

class TiltGuard
{
public:
    static constexpr const char* kParam = "TG_MAX_TILT";

    explicit TiltGuard(mini_px4::System& sys)
        : sys_(sys), attitudeSub_(sys.attitude), paramSub_(sys.parameterUpdate)
    {
        updateParams();
        workId_ = sys_.scheduleOnInterval("tilt_guard", 10000, [this] { run(); });
    }

    ~TiltGuard() { sys_.cancel(workId_); }

    TiltGuard(const TiltGuard&) = delete;
    TiltGuard& operator=(const TiltGuard&) = delete;

    void run()
    {
        mini_px4::Attitude a{};
        if (!attitudeSub_.update(a)) {
            ++noData_;
            return;
        }
        ++cycles_;
        const double deg = 3.14159265358979323846 / 180.0;
        const double c = std::cos(a.rollDeg * deg) * std::cos(a.pitchDeg * deg);
        const auto tilt = static_cast<float>(std::acos(std::clamp(c, -1.0, 1.0)) / deg);
        const bool exceeded = tilt > maxTiltDeg_;
        if (exceeded && !wasExceeded_) {   // count crossings, not samples
            ++exceedCount_;
        }
        wasExceeded_ = exceeded;
        peakDeg_ = std::max(peakDeg_, tilt);
        sys_.tiltStatus.publish(mini_px4::TiltStatus{a.timestampUs, tilt, maxTiltDeg_, exceeded, exceedCount_});
    }

    void printStatus() const
    {
        std::printf("tilt_guard: running, %u cycles, %u without new data, peak tilt %.1f deg, "
                    "threshold %.1f deg, %u exceed events\n",
                    cycles_, noData_, static_cast<double>(peakDeg_), static_cast<double>(maxTiltDeg_),
                    exceedCount_);
    }

private:
    void updateParams()
    {
        float v = 0.0f;
        if (sys_.param(kParam, v)) {
            maxTiltDeg_ = v;
        }
    }

    mini_px4::System& sys_;
    uorb_lite::Subscription<mini_px4::Attitude, 1> attitudeSub_;
    uorb_lite::Subscription<mini_px4::ParameterUpdate, 1> paramSub_;
    int workId_ = -1;
    float maxTiltDeg_ = 30.0f;
    float peakDeg_ = 0.0f;
    bool wasExceeded_ = false;
    std::uint32_t cycles_ = 0;
    std::uint32_t noData_ = 0;
    std::uint32_t exceedCount_ = 0;
};
