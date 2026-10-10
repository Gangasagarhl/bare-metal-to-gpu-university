// uptime.cpp - F12-19 forensic evidence generator: "the logs that disagree".
// Two building-site gateways stamp the same events. Gateway A counts integer milliseconds.
// Gateway B adds 0.1f to a float every 100 ms tick since its own boot. B reboots once.
// Every 6 hours both stamp a sync pulse; at 30 h a door event (B) precedes an alarm (A).
#include <cstdio>

int main()
{
    std::printf("evidence 1: firmware excerpt of gateway B (time keeping)\n");
    std::printf("    static float uptime_s = 0.0f;        // seconds since boot\n");
    std::printf("    void on_tick_100ms() { uptime_s += 0.1f; }\n");
    std::printf("    double stamp() { return boot_epoch_s + uptime_s; }\n\n");
    std::printf("evidence 2: sync pulse stamped by both gateways (site time in s; 6-hourly)\n");
    std::printf("%8s  %28s  %14s  %14s  %10s\n", "pulse", "B uptime, as B counts it (h)",
                "A stamp (s)", "B stamp (s)", "B - A (s)");
    const long kTicksPerDay = 864000;  // 10 ticks per second
    const long kRebootTick = 3 * kTicksPerDay + kTicksPerDay / 2;  // day 3, 12:00
    long a_ms = 0;                       // gateway A: integer milliseconds, exact
    float b_up = 0.0f;                   // gateway B: float seconds since its boot
    double b_epoch = 0.0;                // site time at B's boot
    int pulse = 0;
    for (long tick = 1; tick <= 5 * kTicksPerDay; ++tick) {
        a_ms += 100;
        b_up += 0.1f;
        if (tick == kRebootTick) {
            std::printf("    (gateway B rebooted at site time %.1f s)\n", a_ms / 1000.0);
            b_epoch = a_ms / 1000.0;
            b_up = 0.0f;
        }
        if (tick % (kTicksPerDay / 4) == 0) {
            ++pulse;
            const double a = a_ms / 1000.0;
            const double b = b_epoch + static_cast<double>(b_up);
            std::printf("%8d  %28.1f  %14.1f  %14.3f  %10.3f\n", pulse,
                        static_cast<double>(b_up) / 3600.0, a, b, b - a);
        }
        if (tick == kTicksPerDay + 216000) {  // 30 h: door opened; the alarm rule fires 2 s later
            const double b_door = b_epoch + static_cast<double>(b_up);
            const double a_alarm = a_ms / 1000.0 + 2.0;
            std::printf("    evidence 3: door event (stamped by B) %.3f s; alarm rule fired "
                        "(stamped by A) %.3f s\n", b_door, a_alarm);
        }
    }
    return 0;
}
