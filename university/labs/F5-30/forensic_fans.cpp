// forensic_fans.cpp - evidence generator for the F5-30 forensic lab "Fans at full speed after
// the update". The same model and the same host load as objects.cpp; the only difference is
// the sensor configuration shipped with the "new firmware" (see the answer key).
#include "obmc_model.h"

int main()
{
    Bus bus;
    Journal journal;
    Board board;
    std::cout << "== sensor configuration in the new image\n"
              << "cpu0 adc0 0.1\ncpu1 adc_1 0.1\n\n== journal after the update\n";
    SensorReader reader(bus, journal, board, {{"cpu0", "adc0", 0.1}, {"cpu1", "adc_1", 0.1}});
    ThresholdMonitor monitor(bus, journal, 75.0, 90.0);
    FanControl fans(bus, journal, {"cpu0", "cpu1"});
    for (journal.now = 0; journal.now <= 300; journal.now += 60) {
        const double fan = bus.has("/xyz/openbmc_project/control/fanpwm/fan0")
                               ? bus.get("/xyz/openbmc_project/control/fanpwm/fan0", "Target") : 30.0;
        for (auto& [channel, c] : board.celsius) {
            const double target = 25.0 + 20.0 * (1.0 - 0.6 * fan / 100.0) + (channel == "adc1" ? 3.0 : 0.0);
            c += 0.5 * (target - c);
        }
        reader.poll();
        fans.step();
        if (journal.now % 300 == 0) {
            web_get_sensors(bus, journal);
        }
    }
    std::cout << "\n== objects on the bus at the end\n";
    for (const auto& [path, props] : bus.objects()) {
        for (const auto& [prop, value] : props) {
            std::cout << path << "  " << prop << " = " << value << '\n';
        }
    }
    return 0;
}
