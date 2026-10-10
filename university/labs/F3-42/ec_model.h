// ec_model.h - F3-42 Listing 1: a model of a laptop Embedded Controller (EC) and of the
// operating system's side of the ACPI EC interface. The EC is a small microcontroller with
// its own firmware loop: it scans the keyboard matrix, polls the battery over SMBus, and
// answers the host through two one-byte registers (status/command and data).
// Command codes and status bits follow the ACPI Specification's "ACPI Embedded Controller
// Interface" chapter as recalled by the author (UNVERIFIED: see the chapter's box). The EC
// RAM layout below is this model's own; real laptops define theirs in their ACPI tables.
#pragma once
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace ec {

// Status register bits and commands (ACPI EC interface; unverified in this build).
constexpr uint8_t kOBF = 1u << 0;        // output buffer full: data for the host is waiting
constexpr uint8_t kIBF = 1u << 1;        // input buffer full: the EC has not taken the byte yet
constexpr uint8_t kSCI_EVT = 1u << 5;    // the EC has an event for the host to query
constexpr uint8_t kRD_EC = 0x80, kWR_EC = 0x81, kQR_EC = 0x84;

// This model's EC RAM map (each real EC defines its own, described to the OS in AML).
constexpr uint8_t kBatRemainingLo = 0x10, kBatRemainingHi = 0x11;   // mWh
constexpr uint8_t kBatFullLo = 0x12, kBatFullHi = 0x13;             // mWh
constexpr uint8_t kLidOpen = 0x20;
constexpr uint8_t kQueryLid = 0x51;                                 // event number for "lid"

class Controller {
public:
    explicit Controller(uint32_t seed) : rng_(seed) {}

    // ---- the host's view: two registers --------------------------------------------
    uint8_t readStatus() const { return status_; }
    uint8_t readData() { status_ &= static_cast<uint8_t>(~kOBF); return data_; }
    void writeCommand(uint8_t c) { inbox_.push_back({true, c}); status_ |= kIBF; }
    void writeData(uint8_t d) { inbox_.push_back({false, d}); status_ |= kIBF; }

    // ---- the EC firmware's main loop: one pass per call ------------------------------
    void step()
    {
        ++now_;
        if (now_ % 100 == 0) { pollBattery(); }              // SMBus battery poll
        scanKeyboard();
        if (pendingOutput_ && now_ >= outputAt_) {           // a result is ready for the host
            output(pendingValue_);
            pendingOutput_ = false;
        }
        if ((status_ & kIBF) != 0 && (next() % 4) == 0) {     // host interface: not every pass
            serviceHost();
        }
    }

    // ---- the physical world -----------------------------------------------------------
    void pressKey(int row, int col) { pendingKey_ = row * 16 + col; }
    void closeLid() { ram_[kLidOpen] = 0; events_.push_back(kQueryLid); status_ |= kSCI_EVT; }
    void setBattery(uint16_t remaining, uint16_t full) { smbRemaining_ = remaining; smbFull_ = full; }

    std::deque<uint8_t> keyboardOutput;                      // bytes for the keyboard port
    std::vector<std::string> trace;                          // what the firmware did
    uint32_t now() const { return now_; }

private:
    struct Byte { bool isCommand; uint8_t value; };

    uint32_t next() { rng_ = rng_ * 1664525u + 1013904223u; return rng_ >> 16; }

    void pollBattery()
    {
        // A real EC reads the Smart Battery's registers over SMBus; the model copies them.
        ram_[kBatRemainingLo] = static_cast<uint8_t>(smbRemaining_);
        ram_[kBatRemainingHi] = static_cast<uint8_t>(smbRemaining_ >> 8);
        ram_[kBatFullLo] = static_cast<uint8_t>(smbFull_);
        ram_[kBatFullHi] = static_cast<uint8_t>(smbFull_ >> 8);
    }

    void scanKeyboard()
    {
        if (pendingKey_ < 0) { return; }
        // Debounce: the key must be seen in 3 consecutive scans before it counts.
        if (++debounce_ < 3) { return; }
        const uint8_t code = static_cast<uint8_t>(0x10 + pendingKey_);   // model scan code
        keyboardOutput.push_back(code);
        trace.push_back("t=" + std::to_string(now_) + " EC: key (row " + std::to_string(pendingKey_ / 16) +
                        ", col " + std::to_string(pendingKey_ % 16) + ") debounced, scan code " +
                        std::to_string(code) + " to keyboard port, keyboard interrupt raised");
        pendingKey_ = -1;
        debounce_ = 0;
    }

    void serviceHost()
    {
        const Byte b = inbox_.front();
        inbox_.pop_front();
        if (inbox_.empty()) { status_ &= static_cast<uint8_t>(~kIBF); }
        if (b.isCommand) {
            cmd_ = b.value;
            haveAddr_ = false;
            if (cmd_ == kQR_EC) {
                const uint8_t q = events_.empty() ? 0 : events_.front();
                if (!events_.empty()) { events_.pop_front(); }
                if (events_.empty()) { status_ &= static_cast<uint8_t>(~kSCI_EVT); }
                output(q);
            }
            return;
        }
        if (!haveAddr_) {                                    // first data byte = address
            addr_ = b.value;
            haveAddr_ = true;
            if (cmd_ == kRD_EC) {
                if ((next() % 4) != 0) {
                    output(ram_[addr_]);                     // usually ready at once ...
                } else {
                    pendingValue_ = ram_[addr_];             // ... sometimes 1-2 passes later
                    pendingOutput_ = true;                   // (the firmware was busy)
                    outputAt_ = now_ + 1 + next() % 2;
                }
            }
            return;
        }
        if (cmd_ == kWR_EC) { ram_[addr_] = b.value; }       // second data byte = value
    }

    void output(uint8_t v) { data_ = v; status_ |= kOBF; }

    uint8_t status_ = 0, data_ = 0, cmd_ = 0, addr_ = 0, pendingValue_ = 0;
    bool haveAddr_ = false, pendingOutput_ = false;
    uint32_t outputAt_ = 0;
    std::deque<Byte> inbox_;
    std::deque<uint8_t> events_;
    uint8_t ram_[256] = {};
    uint16_t smbRemaining_ = 0, smbFull_ = 0;
    int pendingKey_ = -1, debounce_ = 0;
    uint32_t now_ = 0, rng_;
};

// The operating system's side: the ACPI EC driver's byte-level handshake.
class HostDriver {
public:
    HostDriver(Controller& c, bool waitForOBF) : ec_(c), waitObf_(waitForOBF) {}
    std::vector<std::string> log;
    bool verbose = false;

    uint8_t read(uint8_t addr)
    {
        waitIbfClear();
        ec_.writeCommand(kRD_EC);
        note("write RD_EC (0x80) to command register");
        waitIbfClear();
        ec_.writeData(addr);
        note("write address to data register");
        if (waitObf_) {
            waitObfSet();
        } else {
            waitIbfClear();                       // BUG (forensic): IBF clear != data ready
        }
        const uint8_t v = ec_.readData();
        note("read data register: " + std::to_string(v));
        return v;
    }

    uint16_t read16(uint8_t lo) { return static_cast<uint16_t>(read(lo) | (read(static_cast<uint8_t>(lo + 1)) << 8)); }

private:
    void waitIbfClear() { while ((ec_.readStatus() & kIBF) != 0) { ec_.step(); } }
    void waitObfSet() { while ((ec_.readStatus() & kOBF) == 0) { ec_.step(); } }
    void note(const std::string& s)
    {
        if (verbose) { log.push_back("t=" + std::to_string(ec_.now()) + " OS: " + s); }
    }

    Controller& ec_;
    bool waitObf_;
};

}  // namespace ec
