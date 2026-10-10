// The status-line formatter of an old ground-station tool, as found (chapter F12-22).
// Input: "key=value" fields separated by ';' (V volts, I amps, T degrees C).
// Two other tools parse its output. Its rules were never written down.
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

namespace legacy {

inline double g_lastVolts = 0.0;  // remembered between calls
inline std::time_t systemClock() { return std::time(nullptr); }
inline std::time_t (*g_clock)() = &systemClock;  // seam: a test may point it elsewhere

inline std::string formatStatus(const std::string& raw)
{
    double volts = g_lastVolts;
    double amps = 0;
    int temp = -999;
    std::size_t pos = 0;
    while (pos < raw.size()) {
        std::size_t end = raw.find(';', pos);
        if (end == std::string::npos) end = raw.size();
        std::string field = raw.substr(pos, end - pos);
        if (field.size() > 2 && field[1] == '=') {
            double v = std::atof(field.c_str() + 2);
            if (field[0] == 'V') volts = v;
            else if (field[0] == 'I') amps = v;
            else if (field[0] == 'T') temp = int(v);
        }
        pos = end + 1;
    }
    g_lastVolts = volts;
    char buf[96];
    int tenths = int(volts * 10);
    std::snprintf(buf, sizeof buf, "%d.%dV %s%.1fA", tenths / 10, tenths % 10,
                  amps < 0 ? "CHG " : "", amps < 0 ? -amps : amps);
    std::string out = buf;
    if (temp != -999) out += " " + std::to_string(temp) + "C";
    if (temp > 60) out += " HOT";
    if (volts < 10.5) out += " LOW";
    out += " @" + std::to_string(g_clock() % 60);  // for the log viewer
    return out;
}

}  // namespace legacy
