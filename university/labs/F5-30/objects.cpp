// objects.cpp - run the OpenBMC-style model of obmc_model.h for ten simulated minutes:
// the host's load rises, the two CPU temperatures follow, and the daemons react only through
// the bus: the threshold monitor logs crossings, fan control raises the fan target, and the
// web front end answers from the bus without talking to any other daemon.
#include "obmc_model.h"

int main()
{
    Bus bus;
    Journal journal;
    Board board;
    SensorReader reader(bus, journal, board, {{"cpu0", "adc0", 0.1}, {"cpu1", "adc1", 0.1}});
    ThresholdMonitor monitor(bus, journal, 75.0, 90.0);
    FanControl fans(bus, journal, {"cpu0", "cpu1"});
    bus.watch([&journal](const std::string& path, const std::string& prop, double v) {
        if (prop == "Target") {
            journal.log("bus-monitor", path + " Target=" + std::to_string(static_cast<int>(v)) + " %");
        }
    });

    for (journal.now = 0; journal.now <= 600; journal.now += 60) {
        // the host's load: idle, then a job from t=120 s to t=420 s (made-up model numbers:
        // 25 C inlet air, heat that the fans remove in proportion to their speed)
        const bool busy = journal.now >= 120 && journal.now < 420;
        const double fan = bus.has("/xyz/openbmc_project/control/fanpwm/fan0")
                               ? bus.get("/xyz/openbmc_project/control/fanpwm/fan0", "Target") : 30.0;
        for (auto& [channel, c] : board.celsius) {
            const double heat = busy ? 120.0 : 20.0;
            const double target = 25.0 + heat * (1.0 - 0.6 * fan / 100.0) + (channel == "adc1" ? 3.0 : 0.0);
            c += 0.5 * (target - c);                // move half way towards the target each minute
        }
        reader.poll();
        fans.step();
        if (journal.now % 300 == 0) {
            web_get_sensors(bus, journal);
        }
    }
    return 0;
}
