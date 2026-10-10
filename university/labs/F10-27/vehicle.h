// vehicle.h - F10-27 Listing 2: the "vehicle code". It includes hal.h and nothing that
// belongs to a platform: no <cstdio>, no registers, no RTOS calls. The same file is compiled
// for the PC (vehicle_host.cpp) and for the microcontroller (vehicle_fw.cc).
#pragma once
#include <stdint.h>

#include "hal.h"

#ifndef LOG_LINE_BYTES
#define LOG_LINE_BYTES 64            // size of the telemetry line buffer (forensic: change it)
#endif

namespace vehicle {

inline const hal::Hal* g = nullptr;  // the one HAL object, set by setup()
inline int32_t lastRate = 0;
inline int32_t lastCommand = 0;
inline uint32_t fastRuns = 0;
inline bool ledOn = false;

// A tiny bounded formatter: freestanding firmware has no printf.
struct Line {
    char* buf;
    uint32_t size;
    uint32_t len = 0;
    void text(const char* s)
    {
        while (*s != '\0' && len + 1 < size) {
            buf[len++] = *s++;
        }
        buf[len] = '\0';
    }
    void number(int32_t v)
    {
        char digits[12];
        int n = 0;
        uint32_t u = v < 0 ? 0u - static_cast<uint32_t>(v) : static_cast<uint32_t>(v);
        do {
            digits[n++] = static_cast<char>('0' + u % 10);
            u /= 10;
        } while (u != 0);
        if (v < 0) {
            digits[n++] = '-';
        }
        while (n > 0 && len + 1 < size) {
            buf[len++] = digits[--n];
        }
        buf[len] = '\0';
    }
};

// Fast loop: read the gyro, run a proportional rate controller, command the actuator.
inline void fastLoop()
{
    const int32_t rate = g->gyro.readRollRate();
    int32_t command = (0 - rate) * 15 / 1000;          // P gain 0.015, target rate 0
    if (command > 500) { command = 500; }
    if (command < -500) { command = -500; }
    g->motor.write(command);
    lastRate = rate;
    lastCommand = command;
    ++fastRuns;
}

// Telemetry: take a snapshot, format it, toggle the LED, print.
inline void telemetry()
{
    char buffer[LOG_LINE_BYTES];
    Line line{buffer, LOG_LINE_BYTES};
    line.text("V tick=");
    line.number(static_cast<int32_t>(g->scheduler.ticks()));
    line.text(" fast_runs=");
    line.number(static_cast<int32_t>(fastRuns));
    line.text(" rate_mdps=");
    line.number(lastRate);
    line.text(" cmd=");
    line.number(lastCommand);
    line.text("\n");
    ledOn = !ledOn;
    g->led.set(ledOn);
    g->console.write(buffer);
}

inline void setup(const hal::Hal& h)
{
    g = &h;
    h.console.write("V vehicle setup: fast loop every tick, telemetry every 10 ticks\n");
    h.scheduler.addPeriodic(fastLoop, 1, 3, "fast");
    h.scheduler.addPeriodic(telemetry, 10, 1, "telem");
}

}  // namespace vehicle
