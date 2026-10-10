// forensic_mission.cpp - evidence pack of the MP7 forensic lab "LAND on the first leg".
// Team B's guard (est_guard_teamb.h) passed their own tests and a SITL hover, then commanded
// LAND on the first leg of the first SITL mission. This program regenerates their evidence.
#include <cstdio>

#include "est_guard_teamb.h"
#include "guard_tests.h"
#include "mp7_sitl.h"

int main()
{
    std::printf("== E1  Team B's own unit tests (their selection)\n");
    mp7::run_suite<mp7::EstGuardTeamB>("unit tests: est_guard_teamb.h (EstGuardTeamB)",
                                       {"U1", "U3", "U4", "U5", "U7", "U8"});

    std::printf("\n== E2  SITL hover, 60 s, no mission (Team B's 'bench' check)\n");
    mp7::Scenario h;
    h.id = "H";
    h.t_max = 60;
    const mp7::Result rh = mp7::fly<mp7::EstGuardTeamB>(h);
    std::printf("faults: %s; max innovation %.2f m\n", mp7::fault_names(rh.faults).c_str(),
                rh.max_innov);

    std::printf("\n== E3  first SITL mission: 2 laps of a 40 m square, wind (1.3, -0.3) m/s\n");
    mp7::Scenario s;
    s.id = "N1";
    s.seed = 1;
    s.wind = {1.3, -0.3, 0};
    s.mission = mp7::laps(2, 40);
    const mp7::Result r = mp7::fly<mp7::EstGuardTeamB>(s);
    for (const auto& e : r.events) std::printf("event %7.2f s  %s\n", e.t, e.text.c_str());
    std::printf("\n  t[s] mode     speed[m/s]  pos_innov[m]  mag_dev\n");
    for (const auto& row : r.trace)
        if (row.t <= 16.0)
            std::printf("%6.2f %-8s %10.2f %13.2f %8.3f\n", row.t, row.mode.c_str(), row.speed,
                        row.pos_innov, row.mag_dev);
    std::printf("parameters: EG_POS_GATE 2.0 m, EG_POS_HOLD 0.3 s, EG_FILT_TAU 0.5 s; "
                "SITL step 10 ms (lockstep)\n");
    return 0;
}
