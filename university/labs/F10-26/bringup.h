// bringup.h - a model of the first boot of a port: the "hardware" below is wired in
// code (which device answers on which chip select or address), the board description
// comes from standard input, and the start-up code probes every described device by
// reading its WHO_AM_I register, as sensor drivers do. Our own model, not PX4 or NuttX.
#pragma once

#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace model_hw {

// The real wiring of the model board: bus/select -> WHO_AM_I value of the chip there.
inline const std::map<std::string, int>& wiring()
{
    static const std::map<std::string, int> w = {
        {"SPI1/PC2", 0x47},   // the first IMU
        {"SPI1/PD7", 0x50},   // the barometer
        {"SPI4/PE4", 0x12},   // the second IMU
        {"I2C1/0x1e", 0x3d},  // the magnetometer
    };
    return w;
}

// Reading a register of a select line where no chip answers returns 0xff: the data
// line is pulled high (an assumption of this model, stated in the chapter).
inline int readWhoAmI(const std::string& busSelect)
{
    auto it = wiring().find(busSelect);
    return it == wiring().end() ? 0xff : it->second;
}

} // namespace model_hw

inline int boot(std::istream& in)
{
    std::map<std::string, int> found;   // sensor kind -> instances that probed correctly
    std::map<std::string, int> wanted;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string kind;
        s >> kind;
        if (kind == "board") {
            std::string name;
            s >> name;
            std::printf("[boot] model board %s\n", name.c_str());
        } else if (kind == "console") {
            std::string uart;
            s >> uart;
            std::printf("[boot] console on %s\n", uart.c_str());
        } else if (kind == "spi" || kind == "i2c") {
            std::string bus, sel, dev, whoText;
            s >> bus >> sel >> dev >> whoText;
            const int expected = std::stoi(whoText, nullptr, 16);
            const int got = model_hw::readWhoAmI(bus + "/" + sel);
            const std::string sensor = dev.substr(0, dev.size() - 1);   // imu0 -> imu
            ++wanted[sensor];
            const bool ok = got == expected;
            std::printf("[probe] %-5s %s %-4s who_am_i 0x%02x expected 0x%02x %s\n", dev.c_str(), bus.c_str(),
                        sel.c_str(), got, expected,
                        ok ? "ok" : (got == 0xff ? "FAILED (no answer)" : "FAILED (another chip answered)"));
            if (ok) {
                ++found[sensor];
            }
        }
    }
    int missing = 0;
    for (const auto& [sensor, n] : wanted) {
        std::printf("[sensors] %-4s %d of %d started\n", sensor.c_str(), found[sensor], n);
        if (found[sensor] == 0) {
            std::printf("[preflight] FAIL: no %s; arming refused\n", sensor.c_str());
            ++missing;
        } else if (found[sensor] < n) {
            std::printf("[preflight] WARN: %s redundancy lost\n", sensor.c_str());
        }
    }
    if (missing == 0) {
        std::printf("[preflight] sensors present\n");
    }
    return 0;
}
