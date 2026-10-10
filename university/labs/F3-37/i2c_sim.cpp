// i2c_sim.cpp - F3-37 Listing 4: test the I2C driver of Listing 1 on the host, against
// simulated open-drain wires, a simulated temperature sensor (our own model, address 0x48)
// and a text "logic analyser" that records SCL and SDA and checks the bus rules.
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "i2c_bitbang.h"

namespace {

// A target (slave) device: pointer register + 16-bit temperature register, MSB first.
class SimSensor {
public:
    explicit SimSensor(uint8_t addr7) : addr_(addr7) {}
    int16_t temperature = 0x1780;              // 23.5 degC in 1/256 degC units
    uint8_t config = 0;

    // Called on every change of the wires; returns the level this device drives on SDA
    // (true = released). The real device does the same with logic clocked by SCL.
    bool onEdge(bool scl, bool sda)
    {
        if (scl && prevScl_ && prevSda_ && !sda) {         // START (or repeated START)
            state_ = State::Address; bits_ = 0; shift_ = 0; drive_ = true;
        } else if (scl && prevScl_ && !prevSda_ && sda) {  // STOP
            state_ = State::Idle; drive_ = true;
        } else if (scl && !prevScl_) {                     // SCL rising: sample
            onRise(sda);
        } else if (!scl && prevScl_) {                     // SCL falling: prepare next bit
            onFall();
        }
        prevScl_ = scl; prevSda_ = sda;
        return drive_;
    }

private:
    enum class State { Idle, Address, Write, Read, Ignore };

    void onRise(bool sda)
    {
        if (state_ == State::Address || state_ == State::Write) {
            if (bits_ < 8) {
                shift_ = static_cast<uint8_t>((shift_ << 1) | (sda ? 1 : 0));
            }
            ++bits_;
        } else if (state_ == State::Read) {
            if (bits_ == 8) {
                masterAck_ = !sda;                         // master ACKs (low) or NACKs
            }
            ++bits_;
        }
    }

    void onFall()
    {
        if ((state_ == State::Address || state_ == State::Write) && bits_ == 8) {
            byteDone();                                    // decide ACK for the 9th clock
        } else if ((state_ == State::Address || state_ == State::Write) && bits_ == 9) {
            drive_ = true; bits_ = 0; shift_ = 0;          // ACK clock over
            if (state_ == State::Write && reading_) {
                state_ = State::Read; nextReadBit();
            }
        } else if (state_ == State::Read) {
            if (bits_ == 9) {
                bits_ = 0;
                if (!masterAck_) { state_ = State::Ignore; drive_ = true; return; }
                ++readIndex_;
            }
            nextReadBit();
        }
    }

    void byteDone()
    {
        if (state_ == State::Address) {
            if ((shift_ >> 1) != addr_) { state_ = State::Ignore; drive_ = true; return; }
            reading_ = (shift_ & 1u) != 0;
            readIndex_ = 0;
            state_ = State::Write;                         // stay here for the ACK clock
            drive_ = false;                                // ACK: pull SDA low
            writeIndex_ = 0;
            return;
        }
        if (writeIndex_ == 0) { pointer_ = shift_; } else if (pointer_ == 1) { config = shift_; }
        ++writeIndex_;
        drive_ = false;                                    // ACK every written byte
    }

    void nextReadBit()
    {
        const uint16_t reg = pointer_ == 0 ? static_cast<uint16_t>(temperature) : config;
        const uint8_t byte = pointer_ == 0 ? static_cast<uint8_t>(readIndex_ == 0 ? reg >> 8 : reg)
                                           : static_cast<uint8_t>(reg);
        drive_ = bits_ < 8 ? ((byte >> (7 - bits_)) & 1u) != 0 : true;
    }

    uint8_t addr_;
    State state_ = State::Idle;
    int bits_ = 0;
    uint8_t shift_ = 0;
    bool reading_ = false;
    bool masterAck_ = false;
    int readIndex_ = 0;
    int writeIndex_ = 0;
    uint8_t pointer_ = 0;
    bool drive_ = true;
    bool prevScl_ = true;
    bool prevSda_ = true;
};

// The wires: each line is low if ANY device pulls it low (wired-AND with a pull-up).
// The analyser keeps every change of the lines (`events`) and the levels at the end of every
// half period (`samples`, used only to draw the waveform).
struct SimPins {
    SimSensor* sensor = nullptr;
    bool masterScl = true, masterSda = true, targetSda = true;
    std::vector<std::pair<bool, bool>> events;
    std::vector<std::pair<bool, bool>> samples;

    bool line() const { return masterSda && targetSda; }
    void update()
    {
        targetSda = sensor->onEdge(masterScl, line());
        targetSda = sensor->onEdge(masterScl, line());  // settle: the target reacts once more
        const std::pair<bool, bool> now{masterScl, line()};
        if (events.empty() || events.back() != now) {
            events.push_back(now);
        }
    }
    void scl(bool high) { masterScl = high; update(); }
    void sda(bool high) { masterSda = high; update(); }
    bool readSda() { return line(); }
    void wait() { samples.emplace_back(masterScl, line()); }
    void clear() { events.clear(); samples.clear(); }
};

std::string wave(const std::vector<std::pair<bool, bool>>& t, bool scl, size_t n)
{
    std::string s;
    for (size_t i = 0; i < n && i < t.size(); ++i) {
        s += (scl ? t[i].first : t[i].second) ? '-' : '_';
    }
    return s;
}

// Bus rule check: while SCL is high, SDA may change only as START (falling) or STOP (rising).
// Every such change is counted, so an illegal data change would show up as an extra START or
// STOP. Also counts SCL rising edges.
void analyse(const std::vector<std::pair<bool, bool>>& t, int& starts, int& stops, int& clocks)
{
    starts = stops = clocks = 0;
    bool scl0 = true, sda0 = true;                     // idle bus before the first event
    for (const auto& [scl1, sda1] : t) {
        if (scl0 && scl1 && sda0 && !sda1) { ++starts; }
        if (scl0 && scl1 && !sda0 && sda1) { ++stops; }
        if (!scl0 && scl1) { ++clocks; }
        scl0 = scl1; sda0 = sda1;
    }
}

}  // namespace

int main()
{
    SimSensor sensor(0x48);
    SimPins pins;
    pins.sensor = &sensor;
    I2cMaster<SimPins> i2c(pins);

    // 1. Read the temperature register: write pointer 0, repeated START, read 2 bytes.
    const uint8_t pointer = 0x00;
    uint8_t raw[2] = {0, 0};
    const bool ok = i2c.transfer(0x48, &pointer, 1, raw, 2);
    const int16_t value = static_cast<int16_t>((raw[0] << 8) | raw[1]);
    std::cout << "read 0x48: " << (ok ? "ACK" : "NACK") << ", bytes 0x" << std::hex
              << int(raw[0]) << " 0x" << int(raw[1]) << std::dec << " -> "
              << value * 100 / 256 << " hundredths of a degree C\n";

    int starts = 0, stops = 0, clocks = 0;
    analyse(pins.events, starts, stops, clocks);
    std::cout << "analyser: " << pins.events.size() << " line changes, " << starts
              << " START (incl. repeated), " << stops << " STOP, " << clocks
              << " SCL rising edges (expect 5 bytes x 9 + 1 repeated START + 1 STOP = 47)\n";
    std::cout << "start of the transfer: START, address 0x48 = 1001000, W = 0, ACK (one char per half period):\n";
    std::cout << "  SCL " << wave(pins.samples, true, 40) << "\n";
    std::cout << "  SDA " << wave(pins.samples, false, 40) << "\n";

    // 2. Wrong address: the classic mistake of passing the 8-bit form 0x90 as a 7-bit address.
    pins.clear();
    uint8_t junk[2] = {0, 0};
    const bool wrong = i2c.transfer(0x90 & 0x7F, &pointer, 1, junk, 2);
    std::cout << "read 0x10 (0x90 truncated to 7 bits): " << (wrong ? "ACK" : "NACK") << "\n";

    // 3. Change the configuration register and read it back.
    const uint8_t cfg[2] = {0x01, 0x60};
    const bool wok = i2c.transfer(0x48, cfg, 2, nullptr, 0);
    const uint8_t p1 = 0x01;
    uint8_t back = 0;
    const bool rok = i2c.transfer(0x48, &p1, 1, &back, 1);
    std::cout << "config write " << (wok ? "ACK" : "NACK") << ", read back 0x" << std::hex
              << int(back) << std::dec << (rok ? " (ACK)" : " (NACK)") << "\n";

    const bool pass = ok && value == 0x1780 && starts == 2 && stops == 1 && clocks == 47 && !wrong &&
                      wok && rok && back == 0x60;
    std::cout << (pass ? "all checks PASS" : "a check FAILED") << "\n";
    return pass ? 0 : 1;
}
