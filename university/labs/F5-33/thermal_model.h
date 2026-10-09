// thermal_model.h - the university's own lumped model of one server's cooling, used by
// fan_loop.cpp (lab) and night.cpp (forensic evidence). Every number here is a MODEL
// parameter chosen for teaching, not a property of any real server, processor or fan.
//   processor power P = 60 W + 1.6 W per % of load            (60 W idle, 220 W at 100 %)
//   airflow a        = sum over working fans of duty/100 / number of fans   (0 .. 1)
//   steady temperature T* = inlet + 0.318 * P / (0.25 + a)    (degrees C)
//   the processor moves towards T* with a time constant of 4 minutes
//   fan controller: duty = 30 % + 3 % per degree above 50 C, limited to 30..100 %
#pragma once
#include <algorithm>
#include <vector>

struct Server
{
    int fans = 4;
    std::vector<bool> fan_ok = std::vector<bool>(4, true);
    double duty = 30.0;        // % for every working fan
    double cpu_c = 40.0;       // processor temperature, degrees C

    double power(double load_pct) const { return 60.0 + 1.6 * load_pct; }

    double airflow() const
    {
        int working = 0;
        for (bool ok : fan_ok) {
            working += ok ? 1 : 0;
        }
        return duty / 100.0 * working / fans;
    }

    double steady(double inlet_c, double load_pct) const
    {
        return inlet_c + 0.318 * power(load_pct) / (0.25 + airflow());
    }

    int rpm(int fan) const     // tachometer reading: 90 rpm per % duty, 0 when failed
    {
        return fan_ok[static_cast<std::size_t>(fan)] ? static_cast<int>(duty * 90.0) : 0;
    }

    // one simulated minute: controller first, then the temperature moves towards T*
    void minute(double inlet_c, double load_pct)
    {
        duty = std::clamp(30.0 + (cpu_c - 50.0) * 3.0, 30.0, 100.0);
        cpu_c += (steady(inlet_c, load_pct) - cpu_c) / 4.0;
    }
};
