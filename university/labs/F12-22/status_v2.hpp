// status_v2.hpp - the formatter after refactoring: state and clock are explicit, parsing and
// formatting are separate. Behaviour must equal legacy_seam.hpp line for line, quirks included.
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <optional>
#include <string>

namespace v2 {

struct Reading
{
    std::optional<double> volts;  // absent: keep the previous voltage (legacy rule)
    double amps = 0;
    std::optional<int> temp;      // truncated toward zero (legacy rule)
};

inline Reading parse(const std::string& raw)
{
    Reading r;
    std::size_t pos = 0;
    while (pos < raw.size()) {
        std::size_t end = raw.find(';', pos);
        if (end == std::string::npos) end = raw.size();
        const std::string field = raw.substr(pos, end - pos);
        if (field.size() > 2 && field[1] == '=') {  // "V=" with no value is ignored
            const double v = std::atof(field.c_str() + 2);
            if (field[0] == 'V') r.volts = v;
            if (field[0] == 'I') r.amps = v;
            if (field[0] == 'T') r.temp = int(v);
        }
        pos = end + 1;
    }
    return r;
}

// Sprouted for the new feature: volts per cell, a new function with its own tests.
inline double cellVolts(double volts, int cells)
{
    return cells > 0 ? volts / cells : volts;
}

class Formatter
{
public:
    explicit Formatter(std::function<std::time_t()> clock) : clock_(std::move(clock)) {}

    std::string format(const std::string& raw, int cells = 0)
    {
        const Reading r = parse(raw);
        lastVolts_ = r.volts.value_or(lastVolts_);
        const int tenths = int(lastVolts_ * 10);  // truncation kept on purpose (F12-22)
        char buf[96];
        std::snprintf(buf, sizeof buf, "%d.%dV %s%.1fA", tenths / 10, tenths % 10,
                      r.amps < 0 ? "CHG " : "", r.amps < 0 ? -r.amps : r.amps);
        std::string out = buf;
        if (r.temp) out += " " + std::to_string(*r.temp) + "C";
        if (r.temp && *r.temp > 60) out += " HOT";
        if (lastVolts_ < 10.5) out += " LOW";
        if (cells > 0) {
            std::snprintf(buf, sizeof buf, " %.2fV/cell", cellVolts(lastVolts_, cells));
            out += buf;
        }
        return out + " @" + std::to_string(clock_() % 60);
    }

private:
    std::function<std::time_t()> clock_;
    double lastVolts_ = 0.0;
};

}  // namespace v2
