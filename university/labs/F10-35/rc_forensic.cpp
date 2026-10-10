// rc_forensic.cpp - evidence generator for the F10-35 forensic lab ("the RC-loss test that
// did nothing"). Course simulator; the receiver model and its "ch5" output are part of this
// program, not of a real receiver. The fault injected is described in the chapter's answer key.
#include "../F10-33/dronesim.hpp"

#include <cstdio>
#include <vector>

int main()
{
    dn::World w;
    w.wind = {1.0, 0.5, 0};
    w.rc_hold_on_loss = true;
    dn::Fsw f;  // rc_timeout 1.0 s, rc_action RTL (the defaults)
    std::printf("parameters: rc_timeout=%.1f s rc_action=%s\n", f.p.rc_timeout,
                dn::name(f.p.rc_action));
    std::vector<dn::V3> m;
    for (int i = 0; i < 3; ++i) {
        m.push_back({40, 0, 10});
        m.push_back({40, 40, 10});
        m.push_back({0, 40, 10});
        m.push_back({0, 0, 10});
    }
    dn::Sensors s = w.sense();
    f.arm(s, m);
    double ch5 = 1500;
    dn::Rng rx(7);
    std::printf("%7s %-8s %8s %6s %7s %8s %8s %9s\n", "t[s]", "mode", "rc_valid", "rc_lq", "ch5",
                "x[m]", "y[m]", "home[m]");
    int k = 0;
    while (w.t < 400.0) {
        if (w.t >= 40.0) w.rc_link = false;  // the test: transmitter switched off at 40 s
        s = w.sense();
        if (w.rc_link) ch5 = 1500 + static_cast<int>(rx.uniform() * 5) - 2;  // live channel jitters
        const dn::Command c = f.step(s);
        w.step(c);
        if (++k % 500 == 0 || (w.t > 39.0 && w.t < 42.6 && k % 50 == 0))
            std::printf("%7.2f %-8s %8d %6d %7.0f %8.2f %8.2f %9.2f\n", w.t, dn::name(f.mode),
                        s.rc_valid ? 1 : 0, s.rc_lq, ch5, w.pos.x, w.pos.y, dn::hnorm(w.pos));
        if (f.mode == dn::Mode::Disarmed || w.crashed) break;
    }
    for (const auto& e : f.events) std::printf("event %7.2f s  %s\n", e.t, e.text.c_str());
    return 0;
}
