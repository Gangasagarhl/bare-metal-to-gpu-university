// hal.h - F10-27 Listing 1: the university's teaching HAL (hardware abstraction layer).
// Vehicle code sees only these abstract classes. Each backend (host simulation, uRTOS on
// the QEMU microcontroller) implements them. The shape follows the idea of ArduPilot's HAL
// (abstract drivers, one backend per platform, one HAL object the vehicle code holds);
// the class and method names here are OUR OWN, not ArduPilot's.
#pragma once
#include <stdint.h>

namespace hal {

class Console {                      // text output (a UART on a board, stdout on a PC)
public:
    virtual void write(const char* text) = 0;
protected:
    ~Console() = default;
};

class Gyro {                         // one axis of an IMU: roll rate in millidegrees/s
public:
    virtual int32_t readRollRate() = 0;
protected:
    ~Gyro() = default;
};

class MotorOut {                     // one actuator command, -500 ... +500
public:
    virtual void write(int32_t command) = 0;
protected:
    ~MotorOut() = default;
};

class Led {
public:
    virtual void set(bool on) = 0;
protected:
    ~Led() = default;
};

class Scheduler {                    // periodic tasks; larger priority number = more urgent
public:
    using Task = void (*)();
    virtual void addPeriodic(Task task, uint32_t periodTicks, uint8_t priority,
                             const char* name) = 0;
    virtual uint32_t ticks() = 0;
    virtual void run(uint32_t stopTick) = 0;     // never returns on the board backend
protected:
    ~Scheduler() = default;
};

struct Hal {                         // everything the vehicle code may touch
    Console& console;
    Gyro& gyro;
    MotorOut& motor;
    Led& led;
    Scheduler& scheduler;
};

}  // namespace hal
