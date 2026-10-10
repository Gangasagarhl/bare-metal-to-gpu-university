// sim_airframe.h - F10-27: a one-axis toy airframe (integer arithmetic only, so that the PC
// and the emulated microcontroller compute exactly the same numbers). Both backends feed
// their Gyro from it, because QEMU's board has no IMU. Exercise model, not a real vehicle.
#pragma once
#include <stdint.h>

namespace sim {

struct Airframe {
    int32_t rollRate = 30000;        // millidegrees/s: a gust has just rolled the vehicle
    void apply(int32_t command)      // one tick: actuator effect minus a little damping
    {
        rollRate = rollRate + 2 * command - rollRate / 10;
    }
};

}  // namespace sim
