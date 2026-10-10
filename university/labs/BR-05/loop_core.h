// loop_core.h - BR-05 Listing 1: the sensor-to-actuator step that every world of this lab
// runs unchanged: bare metal, the RTOS and Linux. It uses only <stdint.h> and integer
// arithmetic, so the same file compiles for the Cortex-M3 (freestanding, no library) and
// for the PC (hosted C++). What differs between the worlds is only WHEN step() is called.
#pragma once
#include <stdint.h>

namespace br05 {

// A simulated heater plate: temperature in milli-degrees Celsius. Each step, heat comes in
// in proportion to the heater command u (0 ... 1000 per mille) and leaks out in proportion
// to the difference from the 20 degC room. A model, not a real device.
struct Plant {
    int32_t tempMilliC = 20000;
};

inline int32_t readSensor(const Plant& p)          // "the sensor": read the temperature
{
    return p.tempMilliC;
}

inline void driveActuator(Plant& p, int32_t u)     // "the actuator": apply the command
{
    p.tempMilliC += (u * 30 - (p.tempMilliC - 20000)) / 50;
}

// A proportional-integral controller with integer gains and an anti-windup clamp.
struct PiController {
    int32_t setpointMilliC;
    int32_t integral = 0;

    int32_t step(int32_t measured)
    {
        const int32_t error = setpointMilliC - measured;
        integral += error;
        if (integral > 400000) {
            integral = 400000;
        }
        if (integral < -400000) {
            integral = -400000;
        }
        int32_t u = error / 40 + integral / 600;
        if (u > 1000) {
            u = 1000;
        }
        if (u < 0) {
            u = 0;
        }
        return u;
    }
};

// One control step = sense, compute, actuate. Returns the command so that callers can
// time-stamp the actuation and fold the command into a checksum.
struct Loop {
    Plant plant;
    PiController pi{40000};                          // hold the plate at 40 degC
    uint32_t checksum = 2166136261u;                 // FNV-1a over every command

    int32_t step()
    {
        const int32_t measured = readSensor(plant);
        const int32_t u = pi.step(measured);
        driveActuator(plant, u);
        checksum = (checksum ^ static_cast<uint32_t>(u)) * 16777619u;
        return u;
    }
};

}  // namespace br05
