// board_check.h - reads this course's plain-text board description and checks the
// mistakes that most often break a first port: a pin used twice, two devices on one
// chip select or I2C address, PWM outputs on one timer asking for different rates,
// and no console. The format is ours (see board_check.in); it is not PX4's.
//
//   board <name>
//   console <uart> <tx pin> <rx pin>
//   spi <bus> <cs pin> <device> <who_am_i>
//   i2c <bus> <address> <device> <who_am_i>
//   pwm <output> <timer> <channel> <pin> <rate_hz>
//   led <name> <pin>
#pragma once

#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct BoardReport
{
    int errors = 0;
    int devices = 0;
    int outputs = 0;
};

inline BoardReport checkBoard(std::istream& in)
{
    BoardReport r;
    std::map<std::string, std::string> pinOwner;           // pin -> what uses it
    std::map<std::string, std::string> spiCs;              // bus/cs -> device
    std::map<std::string, std::string> i2cAddr;            // bus/address -> device
    std::map<std::string, std::pair<int, std::string>> timerRate;   // timer -> first rate, output
    bool console = false;
    std::string line;
    int lineNo = 0;

    auto error = [&](const std::string& msg) {
        std::printf("  line %2d: ERROR %s\n", lineNo, msg.c_str());
        ++r.errors;
    };
    auto usePin = [&](const std::string& pin, const std::string& who) {
        auto [it, fresh] = pinOwner.emplace(pin, who);
        if (!fresh) {
            error("pin " + pin + " used by " + who + " is already used by " + it->second);
        }
    };

    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream s(line);
        std::string kind;
        s >> kind;
        if (kind == "board") {
            std::string name;
            s >> name;
            std::printf("board %s\n", name.c_str());
        } else if (kind == "console") {
            std::string uart, tx, rx;
            s >> uart >> tx >> rx;
            usePin(tx, uart + " TX");
            usePin(rx, uart + " RX");
            console = true;
        } else if (kind == "spi") {
            std::string bus, cs, dev, who;
            s >> bus >> cs >> dev >> who;
            usePin(cs, dev + " chip select");
            auto [it, fresh] = spiCs.emplace(bus + "/" + cs, dev);
            if (!fresh) {
                error(dev + " and " + it->second + " share chip select " + cs + " on " + bus);
            }
            ++r.devices;
        } else if (kind == "i2c") {
            std::string bus, addr, dev, who;
            s >> bus >> addr >> dev >> who;
            auto [it, fresh] = i2cAddr.emplace(bus + "/" + addr, dev);
            if (!fresh) {
                error(dev + " and " + it->second + " share address " + addr + " on " + bus);
            }
            ++r.devices;
        } else if (kind == "pwm") {
            std::string out, timer, ch, pin;
            int rate = 0;
            s >> out >> timer >> ch >> pin >> rate;
            usePin(pin, "PWM output " + out);
            // All channels of one timer share its counter, so they share one period.
            auto [it, fresh] = timerRate.emplace(timer, std::make_pair(rate, out));
            if (!fresh && it->second.first != rate) {
                error("output " + out + " wants " + std::to_string(rate) + " Hz but " + timer +
                      " already runs at " + std::to_string(it->second.first) + " Hz for output " +
                      it->second.second);
            }
            ++r.outputs;
        } else if (kind == "led") {
            std::string name, pin;
            s >> name >> pin;
            usePin(pin, name);
        } else {
            error("unknown keyword '" + kind + "'");
        }
    }
    lineNo = 0;
    if (!console) {
        error("no console: the first thing a port needs is a serial console");
    }
    std::printf("%d device(s), %d PWM output(s), %d error(s): %s\n", r.devices, r.outputs, r.errors,
                r.errors == 0 ? "description is consistent" : "fix the description before building");
    return r;
}
