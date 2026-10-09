// F1-71: the university's power-distribution model of a small robot (PRETEND values).
// Pack: 10.8 V open-circuit, 0.15 ohm internal resistance. A shared wire of
// 0.10 ohm carries the motor current. The microcontroller (MCU) board's regulator
// needs at least 7.0 V at its input; below that the MCU is held in reset (brown-out).
// After the voltage is back, the MCU needs 100 ms to boot, then starts the motors.
// The two drive motors are modelled as one equivalent DC motor (F1-69 equations).
#pragma once
#include <cstddef>
#include <cstdio>
#include <string>  // std::char_traits

struct PowerConfig
{
    bool mcuFromPackTerminals = false;  // false: MCU fed after the shared wire
    double softStartMs = 0.0;           // 0: full duty at once
    bool print = true;
};

inline int simulatePower(const PowerConfig& c)
{
    const double vOcv = 10.8;
    const double rPack = 0.15;
    const double rShared = 0.10;
    const double brownOut = 7.0;
    const double bootMs = 100.0;
    // equivalent motor
    const double rM = 0.4;
    const double lM = 0.5e-3;
    const double k = 0.02;
    const double j = 2.0e-4;
    const double b = 1.0e-5;
    const double load = 0.05;  // N m
    const double dt = 1.0e-5;

    double i = 0.0;
    double w = 0.0;
    bool running = true;      // MCU state
    double bootTimer = 0.0;   // ms since MCU (re)started running
    double minV = 99.0;
    int resets = 0;
    char events[1024] = "";
    std::size_t used = 0;
    if (c.print) {
        std::printf("detailed trace, 98-112 ms (every 0.5 ms):\n");
        std::printf("%7s %9s %9s %9s %6s %7s\n", "t ms", "motor A", "pack V", "MCU in V", "MCU",
                    "duty");
    }
    for (long n = 0; n <= 120000; ++n) {  // 1.2 s
        const double tMs = n * dt * 1000.0;
        double duty = 0.0;
        if (running && bootTimer >= bootMs) {
            const double since = bootTimer - bootMs;
            duty = c.softStartMs > 0.0 && since < c.softStartMs ? since / c.softStartMs : 1.0;
        }
        // Bus voltage depends on the current already flowing (resistances in series).
        const double vPack = vOcv - i * rPack;
        const double vBus = vPack - i * rShared;
        const double vMcu = c.mcuFromPackTerminals ? vPack : vBus;
        minV = vMcu < minV ? vMcu : minV;
        if (running && vMcu < brownOut) {
            running = false;
            ++resets;
            if (c.print && resets <= 12) {
                std::snprintf(events + used, sizeof events - used,
                              "  %7.2f ms  RESET (MCU input %.3f V, motor %.2f A)\n", tMs, vMcu,
                              i);
                used = std::char_traits<char>::length(events);
            }
        } else if (!running && vMcu >= brownOut) {
            running = true;
            bootTimer = 0.0;
        }
        if (running) {
            bootTimer += dt * 1000.0;
        }
        // Motor driven by duty x bus voltage (averaged PWM). With duty 0 the bridge
        // is off and the motor current decays towards zero.
        const double vMotor = duty * vBus;
        double di = (vMotor - rM * i - k * w) / lM;
        if (duty == 0.0 && i <= 0.0) {
            di = 0.0;
            i = 0.0;
        }
        double torque = k * i - b * w - (w > 0.0 ? load : 0.0);
        if (w <= 0.0 && torque < 0.0) {
            torque = 0.0;
        }
        i += di * dt;
        w += torque / j * dt;
        if (c.print && n >= 9800 && n <= 11200 && n % 50 == 0) {
            std::printf("%7.1f %9.2f %9.2f %9.2f %6s %6.0f%%\n", tMs, i, vPack, vMcu,
                        running ? (bootTimer >= bootMs ? "RUN" : "BOOT") : "RESET", duty * 100.0);
        }
    }
    if (c.print) {
        std::printf("\nevent list, whole 1.2 s run (first 12 resets):\n%s", events);
        std::printf("brown-out resets: %d, lowest MCU input voltage %.2f V\n", resets, minV);
    }
    return resets;
}
