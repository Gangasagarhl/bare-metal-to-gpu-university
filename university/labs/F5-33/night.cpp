// night.cpp - evidence generator for the forensic lab of F5-33 (the DS304 course forensic
// "The server that reboots at night"). It simulates one server minute by minute from Monday
// 12:00 to Thursday 07:00 with thermal_model.h and writes what an operator would collect:
// the BMC's system event log (SEL), the BMC's sensor history around the nights, and the
// operating system's boot log. Simulation only (the university's model, not a real BMC);
// what was injected is in the answer key of F5-33.
#include "thermal_model.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

const char* kDays[] = {"Mon", "Tue", "Wed", "Thu"};

std::string stamp(int t)  // minutes since Monday 00:00 -> "Tue 02:31"
{
    char buf[16];
    std::snprintf(buf, sizeof buf, "%s %02d:%02d", kDays[t / 1440], t % 1440 / 60, t % 60);
    return buf;
}

struct Log
{
    std::vector<std::string> sel, os, sensors;
    int next_id = 1;
    void event(int t, const std::string& sensor, const std::string& what)
    {
        char buf[160];
        std::snprintf(buf, sizeof buf, "%4d | %s | %-23s | %s", next_id++, stamp(t).c_str(), sensor.c_str(), what.c_str());
        sel.push_back(buf);
    }
};

}  // namespace

int main()
{
    Server s;
    Log log;
    bool on = true, unc = false, uc = false, psu_lost = false;
    int uc_minutes = 0, off_since = -1, boot_done = -1;
    int batch_done_night = -1;              // the night (day index) whose batch already ran
    double inlet = 22.0;
    log.os.push_back(stamp(17 * 60 + 5) + "  os: clean reboot after a kernel package update");
    for (int t = 12 * 60; t <= 3 * 1440 + 7 * 60; ++t) {
        const int day = t / 1440, hm = t % 1440;
        // data hall: the inlet air warms to 26 C between 00:00 and 06:00, 22 C otherwise
        const double want_inlet = hm < 6 * 60 ? 26.0 : 22.0;
        inlet += std::clamp(want_inlet - inlet, -0.1, 0.1);
        // events outside the server's control
        if (t == 1440 + 10 * 60 + 15) { psu_lost = true; log.event(t, "PS2 Status", "power supply input lost, asserted"); }
        if (t == 1440 + 10 * 60 + 16 && psu_lost) { psu_lost = false; log.event(t, "PS2 Status", "power supply input lost, deasserted"); }
        if (t == 1440 + 15 * 60 + 12) { s.fan_ok[2] = false; log.event(t, "Fan 3", "lower critical going low, asserted (0 rpm)"); }
        if (t == 17 * 60 + 5) { log.event(t, "System Event", "OS-initiated warm reset"); }
        // workload
        double load = 0.0;
        if (on) {
            load = (hm >= 8 * 60 && hm < 20 * 60) ? 45.0 : 20.0;
            const bool batch_window = hm >= 2 * 60 && hm < 2 * 60 + 50;
            if (batch_window && batch_done_night != day && boot_done < 0) {
                load = 100.0;               // the nightly backup-and-compress job
                if (hm == 2 * 60 + 49) { batch_done_night = day; }
            }
            if (boot_done >= 0 && t >= boot_done) { boot_done = -1; }
        }
        // physics and fan control (when off: no power, fans stop)
        if (on) {
            s.minute(inlet, load);
        } else {
            s.cpu_c += (inlet - s.cpu_c) / 4.0;
        }
        // BMC threshold logic (this model's thresholds: upper non-critical 85 C, upper critical 95 C)
        if (on) {
            if (!unc && s.cpu_c >= 85.0) { unc = true; log.event(t, "CPU Temp", "upper non-critical going high, asserted"); }
            if (unc && s.cpu_c < 83.0) { unc = false; log.event(t, "CPU Temp", "upper non-critical going high, deasserted"); }
            if (!uc && s.cpu_c >= 95.0) { uc = true; log.event(t, "CPU Temp", "upper critical going high, asserted"); }
            uc_minutes = uc ? uc_minutes + 1 : 0;
            if (uc && uc_minutes >= 2) {    // thermal protection: power the host off
                log.event(t, "System ACPI Power State", "S5/G2 soft-off (thermal protection)");
                on = false; off_since = t; uc = false; unc = false; uc_minutes = 0;
                batch_done_night = day;     // the job died with the host; cron starts it tomorrow
            }
        } else if (t - off_since >= 5) {    // power restore policy "always on" after 5 minutes
            on = true;
            log.event(t, "System ACPI Power State", "S0/G0 working (power restored by BMC policy)");
            log.os.push_back(stamp(t + 3) + "  os: boot; previous shutdown was unexpected (no clean shutdown record)");
            boot_done = t + 3;
        }
        // sensor history: every 10 minutes from 01:30 to 03:30, and at 15:00-15:30 on Tuesday
        const bool night_window = hm >= 90 && hm <= 210 && hm % 10 == 0;
        const bool tuesday_window = day == 1 && hm >= 15 * 60 && hm <= 15 * 60 + 30 && hm % 10 == 0;
        if (night_window || tuesday_window) {
            char buf[160];
            std::snprintf(buf, sizeof buf, "%s  %5.1f  %5.1f  %5d %5d %5d %5d  %4.0f  %s", stamp(t).c_str(),
                          inlet, s.cpu_c, on ? s.rpm(0) : 0, on ? s.rpm(1) : 0, on ? s.rpm(2) : 0, on ? s.rpm(3) : 0,
                          load, on ? "on" : "off");
            log.sensors.push_back(buf);
        }
    }
    std::printf("== evidence 1: BMC system event log (SEL)\n  id | time      | sensor                  | event\n");
    for (const auto& l : log.sel) { std::printf("%s\n", l.c_str()); }
    std::printf("\n== evidence 2: BMC sensor history (C, C, rpm x4, %% host load, host power)\n");
    std::printf("time       inlet    cpu   fan1  fan2  fan3  fan4  load  power\n");
    for (const auto& l : log.sensors) { std::printf("%s\n", l.c_str()); }
    std::printf("\n== evidence 3: operating system boot log\n");
    for (const auto& l : log.os) { std::printf("%s\n", l.c_str()); }
    std::printf("\n== evidence 4: crontab of the host\n0 2 * * *  /usr/local/bin/backup-and-compress   (about 50 minutes at full load)\n");
    return 0;
}
