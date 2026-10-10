// forensic_key.cpp - ANSWER KEY of the MP7 forensic lab. Run only after your analysis.
// 1. our full unit suite against Team B's guard; 2. innovation versus speed, both guards;
// 3. the same first mission with the fixed guard. Exit code 1 by design (Team B fails).
#include <cstdio>

#include "est_guard_teamb.h"
#include "guard_tests.h"
#include "mp7_sitl.h"

template <class G>
double steady_innov(double speed, double dt)
{
    mp7::Run<G> r(mp7::GuardParams{}, dt);
    r.st.vel = {speed, 0, 0};
    r.for_s(20);
    double sum = 0;
    int n = 0;
    while (r.st.t < 30.0) {
        r.gs = r.g.update(r.st.next());
        sum += r.gs.pos_innov;
        ++n;
    }
    return sum / n;
}

int main()
{
    const int fails =
        mp7::run_suite<mp7::EstGuardTeamB>("K1  full suite against est_guard_teamb.h (EstGuardTeamB)");

    std::printf("\nK2  mean horizontal innovation [m] in steady flight (gate 2.0 m)\n");
    std::printf("speed[m/s]  ours@100Hz  teamB@100Hz  teamB@50Hz\n");
    for (double v : {0.0, 1.0, 2.0, 3.0, 4.0, 5.0})
        std::printf("%10.1f %11.2f %12.2f %11.2f\n", v, steady_innov<mp7::EstGuard>(v, 0.01),
                    steady_innov<mp7::EstGuardTeamB>(v, 0.01),
                    steady_innov<mp7::EstGuardTeamB>(v, 0.02));

    std::printf("\nK3  the same first mission with the fixed guard (dt from timestamps)\n");
    mp7::Scenario s;
    s.seed = 1;
    s.wind = {1.3, -0.3, 0};
    s.mission = mp7::laps(2, 40);
    const mp7::Result r = mp7::fly<mp7::EstGuard>(s);
    std::printf("faults: %s; mission done: %s; landed %.1f m from home; max innovation %.2f m\n",
                mp7::fault_names(r.faults).c_str(), r.mission_done ? "yes" : "no", r.land_dist_home,
                r.max_innov);
    return fails == 0 ? 0 : 1;
}
