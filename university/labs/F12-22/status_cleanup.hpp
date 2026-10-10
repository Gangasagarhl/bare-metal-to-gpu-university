// status_cleanup.hpp - a colleague's "harmless cleanup" of the legacy formatter, merged last week.
// Commit message: "tidy formatStatus: no globals, proper rounding, simpler checks; tests pass".
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

namespace cleanup {

inline std::time_t (*g_clock)() = nullptr;

inline std::string formatStatus(const std::string& raw)
{
    double volts = 0.0;
    double amps = 0.0;
    int temp = -999;
    std::size_t pos = 0;
    while (pos < raw.size()) {
        std::size_t end = raw.find(';', pos);
        if (end == std::string::npos) end = raw.size();
        const std::string field = raw.substr(pos, end - pos);
        if (field.size() > 2 && field[1] == '=') {
            const double v = std::atof(field.c_str() + 2);
            if (field[0] == 'V') volts = v;
            if (field[0] == 'I') amps = v;
            if (field[0] == 'T') temp = int(v);
        }
        pos = end + 1;
    }
    const long tenths = std::lround(volts * 10);
    char buf[96];
    std::snprintf(buf, sizeof buf, "%ld.%ldV %s%.1fA", tenths / 10, tenths % 10,
                  amps < 0 ? "CHG " : "", std::fabs(amps));
    std::string out = buf;
    if (temp != -999) out += " " + std::to_string(temp) + "C";
    if (temp >= 60) out += " HOT";
    if (volts < 10.5) out += " LOW";
    return out + " @" + std::to_string(g_clock() % 60);
}

}  // namespace cleanup
