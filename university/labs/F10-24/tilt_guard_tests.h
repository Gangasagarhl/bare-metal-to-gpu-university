// tilt_guard_tests.h - unit tests for a TiltGuard module (include the module first).
// Each test builds a fresh runtime, publishes chosen attitudes, calls run() directly
// and checks what the module published.
#pragma once

#include <cmath>
#include <cstdio>

#include "mini_px4.h"

namespace {

int failures = 0;

void check(bool ok, const char* what)
{
    std::printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) {
        ++failures;
    }
}

mini_px4::TiltStatus step(mini_px4::System& sys, TiltGuard& g, float roll, float pitch)
{
    sys.nowUs += 10000;
    sys.attitude.publish(mini_px4::Attitude{sys.nowUs, roll, pitch});
    g.run();
    uorb_lite::Subscription<mini_px4::TiltStatus, 1> sub(sys.tiltStatus);
    mini_px4::TiltStatus s{};
    sub.update(s);
    return s;
}

} // namespace

inline int runTests()
{
    {
        mini_px4::System sys;
        sys.defineParam(TiltGuard::kParam, 30.0f);
        TiltGuard g(sys);
        const auto s = step(sys, g, 0.0f, 0.0f);
        check(std::fabs(s.tiltDeg) < 1e-3f && !s.exceeded, "level attitude: tilt 0, not exceeded");
        const auto r = step(sys, g, 20.0f, 0.0f);
        check(std::fabs(r.tiltDeg - 20.0f) < 1e-3f, "roll only: tilt equals roll");
    }
    {
        mini_px4::System sys;
        sys.defineParam(TiltGuard::kParam, 30.0f);
        TiltGuard g(sys);
        step(sys, g, 35.0f, 0.0f);
        step(sys, g, 36.0f, 0.0f);
        const auto s = step(sys, g, 37.0f, 0.0f);
        check(s.exceeded && s.exceedCount == 1, "three samples above the limit count as one event");
        step(sys, g, 10.0f, 0.0f);
        const auto t = step(sys, g, 40.0f, 0.0f);
        check(t.exceedCount == 2, "a new crossing counts again");
    }
    {
        mini_px4::System sys;
        sys.defineParam(TiltGuard::kParam, 30.0f);
        TiltGuard g(sys);
        check(!step(sys, g, 25.0f, 0.0f).exceeded, "25 deg is below the default 30 deg");
        sys.setParam(TiltGuard::kParam, 20.0f);
        const auto s = step(sys, g, 25.0f, 0.0f);
        check(s.exceeded && std::fabs(s.maxTiltDeg - 20.0f) < 1e-6f, "a parameter change takes effect");
    }
    {
        mini_px4::System sys;
        sys.defineParam(TiltGuard::kParam, 30.0f);
        TiltGuard g(sys);
        sys.nowUs += 10000;
        g.run();   // no attitude yet
        check(sys.tiltStatus.generation() == 0, "no attitude, no output");
    }
    {
        mini_px4::System sys;
        sys.defineParam(TiltGuard::kParam, 30.0f);
        {
            TiltGuard g(sys);
            sys.runFor(0.1);
        }
        const auto before = sys.tiltStatus.generation();
        sys.runFor(0.1);
        check(before == 10 && sys.tiltStatus.generation() == before, "runs at 100 Hz; nothing after stop");
    }
    std::printf("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
