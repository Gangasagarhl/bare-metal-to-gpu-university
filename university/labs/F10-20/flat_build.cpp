// flat_build.cpp - what "one address space for everything" means, in a model.
// A 256-byte simulated RAM holds the data of two tasks, placed next to each other
// as a linker would place them. The logger copies text into its 32-byte line buffer
// without a length check. In the FLAT model nothing stops the overflow; in the
// PROTECTED model every write is checked against the writing task's region, the
// way a memory protection unit checks accesses (this is a model, not an MPU).
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

enum class Model { Flat, Protected };

struct Region
{
    const char* owner;
    std::size_t base;
    std::size_t size;
};

class Ram
{
public:
    explicit Ram(Model m) : model_(m) {}

    // Returns false (and records a fault) if the write leaves the task's region.
    bool write(const Region& r, std::size_t addr, std::uint8_t value)
    {
        if (model_ == Model::Protected && (addr < r.base || addr >= r.base + r.size)) {
            faultAddr_ = addr;
            return false;
        }
        bytes_.at(addr) = value;
        return true;
    }

    float readFloat(std::size_t addr) const
    {
        float f = 0.0f;
        std::memcpy(&f, &bytes_.at(addr), sizeof f);
        return f;
    }

    void writeFloat(std::size_t addr, float f)
    {
        std::memcpy(&bytes_.at(addr), &f, sizeof f);
    }

    std::size_t faultAddr() const { return faultAddr_; }

    std::uint8_t byte(std::size_t addr) const { return bytes_.at(addr); }

private:
    Model model_;
    std::array<std::uint8_t, 256> bytes_{};
    std::size_t faultAddr_ = 0;
};

// The "link map": where each task's data lives.
constexpr Region kLoggerLine{"logger.line", 0x40, 32};
constexpr Region kRateGains{"rate.gains", 0x60, 12};   // kp, ki, kd as three floats

// The bug: copies the whole message, whatever its length (like an unchecked strcpy).
bool loggerFormat(Ram& ram, const std::string& text)
{
    for (std::size_t i = 0; i <= text.size(); ++i) {   // includes the terminating 0
        const auto c = static_cast<std::uint8_t>(i < text.size() ? text[i] : '\0');
        if (!ram.write(kLoggerLine, kLoggerLine.base + i, c)) {
            return false;
        }
    }
    return true;
}

void run(Model model)
{
    Ram ram(model);
    ram.writeFloat(kRateGains.base + 0, 0.15f);   // kp
    ram.writeFloat(kRateGains.base + 4, 0.20f);   // ki
    ram.writeFloat(kRateGains.base + 8, 0.003f);  // kd
    const std::vector<std::string> messages = {
        "boot ok", "arming checks passed", "takeoff", "alt 1.0 m",
        "alt 2.0 m", "hover", "battery 15.8 V 12.1 A est. 71% remaining", "hover",
    };
    bool loggerAlive = true;
    std::printf("%s model\n", model == Model::Flat ? "FLAT" : "PROTECTED");
    std::printf("cycle  logger message (bytes incl. 0)          rate kp\n");
    for (std::size_t cycle = 0; cycle < messages.size(); ++cycle) {
        const std::string& m = messages[cycle];
        std::string note;
        if (loggerAlive && !loggerFormat(ram, m)) {
            loggerAlive = false;
            char buf[96];
            std::snprintf(buf, sizeof buf, "  <- FAULT: logger wrote 0x%02zx, outside %s; logger stopped",
                          ram.faultAddr(), kLoggerLine.owner);
            note = buf;
        }
        const float kp = ram.readFloat(kRateGains.base);   // the rate controller's read
        std::printf("%5zu  %-36s (%2zu)  %.6g%s\n", cycle, loggerAlive || !note.empty() ? m.c_str() : "-",
                    m.size() + 1, static_cast<double>(kp), note.c_str());
    }
    std::printf("%s bytes at the end:", kRateGains.owner);   // a memory dump, as a debugger shows it
    std::string text;
    for (std::size_t a = kRateGains.base; a < kRateGains.base + kRateGains.size; ++a) {
        const std::uint8_t b = ram.byte(a);
        std::printf(" %02x", b);
        text += (b >= 0x20 && b < 0x7f) ? static_cast<char>(b) : '.';
    }
    std::printf("  |%s|\n\n", text.c_str());
}

} // namespace

int main()
{
    std::printf("link map: %s at 0x%02zx (%zu bytes), %s at 0x%02zx (%zu bytes)\n\n",
                kLoggerLine.owner, kLoggerLine.base, kLoggerLine.size,
                kRateGains.owner, kRateGains.base, kRateGains.size);
    run(Model::Flat);
    run(Model::Protected);
    return 0;
}
