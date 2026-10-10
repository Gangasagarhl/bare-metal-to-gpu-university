// i2c_bitbang.h - F3-37 Listing 1: an I2C controller (master) written once, used twice:
// by the firmware (pins = the emulated board's SBCon register) and by the host test
// (pins = simulated wires with a logic analyser). Bus rules: NXP UM10204, "I2C-bus
// specification and user manual" (pending verification, see the chapter's sources).
#pragma once
#include <stdint.h>

// Pins must provide: void scl(bool high); void sda(bool high); bool readSda(); void wait();
// "high" means "release the open-drain line"; the pull-up resistor makes it 1 unless
// someone else holds it low.
template <typename Pins>
class I2cMaster {
public:
    explicit I2cMaster(Pins& pins) : p_(pins) {}

    // Write `wlen` bytes, then (if rlen > 0) a repeated START and read `rlen` bytes.
    // Returns false if any byte was not acknowledged (NACK).
    bool transfer(uint8_t addr7, const uint8_t* w, int wlen, uint8_t* r, int rlen)
    {
        bool ok = true;
        start();
        if (wlen > 0 || rlen == 0) {
            ok = writeByte(static_cast<uint8_t>(addr7 << 1));          // R/W bit 0 = write
            for (int i = 0; ok && i < wlen; ++i) {
                ok = writeByte(w[i]);
            }
            if (ok && rlen > 0) {
                start();                                                // repeated START
            }
        }
        if (ok && rlen > 0) {
            ok = writeByte(static_cast<uint8_t>((addr7 << 1) | 1u));  // R/W bit 1 = read
            for (int i = 0; ok && i < rlen; ++i) {
                r[i] = readByte(i + 1 < rlen);                          // ACK all but the last
            }
        }
        stop();
        return ok;
    }

    bool probe(uint8_t addr7) { return transfer(addr7, nullptr, 0, nullptr, 0); }

private:
    void start()   // SDA falls while SCL is high
    {
        p_.sda(true);  p_.wait();
        p_.scl(true);  p_.wait();
        p_.sda(false); p_.wait();
        p_.scl(false); p_.wait();
    }

    void stop()    // SDA rises while SCL is high
    {
        p_.sda(false); p_.wait();
        p_.scl(true);  p_.wait();
        p_.sda(true);  p_.wait();
    }

    bool writeByte(uint8_t b)
    {
        for (int bit = 7; bit >= 0; --bit) {       // most significant bit first
            p_.sda(((b >> bit) & 1u) != 0);        // change SDA only while SCL is low
            p_.wait();
            p_.scl(true);  p_.wait();              // receiver samples while SCL is high
            p_.scl(false);
        }
        p_.sda(true);  p_.wait();                  // release SDA for the 9th (ACK) clock
        p_.scl(true);  p_.wait();
        const bool ack = !p_.readSda();            // ACK = receiver holds SDA low
        p_.scl(false); p_.wait();
        return ack;
    }

    uint8_t readByte(bool ack)
    {
        uint8_t b = 0;
        p_.sda(true);                              // release SDA: the target drives it
        for (int bit = 7; bit >= 0; --bit) {
            p_.wait();
            p_.scl(true);  p_.wait();
            b = static_cast<uint8_t>((b << 1) | (p_.readSda() ? 1u : 0u));
            p_.scl(false);
        }
        p_.sda(!ack);  p_.wait();                  // our ACK (low) or NACK (high)
        p_.scl(true);  p_.wait();
        p_.scl(false); p_.wait();
        p_.sda(true);
        return b;
    }

    Pins& p_;
};
