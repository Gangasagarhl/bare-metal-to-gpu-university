// obmc_model.h - the university's own small model of the ARCHITECTURE that the OpenBMC
// documentation describes: separate daemons that never call each other directly but publish
// objects with properties on a shared message bus (D-Bus in OpenBMC), watch each other's
// property changes, and are configured by data files. It is NOT OpenBMC code: no D-Bus, no
// systemd, and the object paths imitate OpenBMC's naming only as an illustration (F5-30,
// unverified box). Used by objects.cpp (lab) and forensic_fans.cpp (forensic evidence).
#pragma once
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// ---- the bus: objects at paths, each with named numeric properties; watchers get changes
class Bus
{
public:
    using Watcher = std::function<void(const std::string& path, const std::string& prop, double value)>;

    void set(const std::string& path, const std::string& prop, double value)
    {
        double& slot = objects_[path][prop];
        const bool changed = slot != value || !std::isfinite(value);
        slot = value;
        if (changed) {
            for (const Watcher& w : watchers_) {
                w(path, prop, value);
            }
        }
    }
    bool has(const std::string& path) const { return objects_.count(path) != 0; }
    double get(const std::string& path, const std::string& prop) const { return objects_.at(path).at(prop); }
    void watch(Watcher w) { watchers_.push_back(std::move(w)); }
    const std::map<std::string, std::map<std::string, double>>& objects() const { return objects_; }

private:
    std::map<std::string, std::map<std::string, double>> objects_;
    std::vector<Watcher> watchers_;
};

// ---- a shared "journal": every daemon logs here with the simulated time
struct Journal
{
    int now = 0;  // seconds since the model started
    void log(const std::string& daemon, const std::string& text)
    {
        std::cout << "t=" << std::setw(4) << now << "s " << std::left << std::setw(16) << daemon
                  << std::right << text << '\n';
    }
};

// ---- the simulated board: ADC channels whose raw counts follow a temperature
struct Board
{
    std::map<std::string, double> celsius{{"adc0", 35.0}, {"adc1", 35.0}};
    // raw count = celsius * 10 (a made-up conversion for this model only)
    bool read_raw(const std::string& channel, int& raw) const
    {
        const auto it = celsius.find(channel);
        if (it == celsius.end()) {
            return false;                           // no such channel on this board
        }
        raw = static_cast<int>(it->second * 10.0);
        return true;
    }
};

// ---- daemon 1: sensor reader, configured by entries "name channel scale"
struct SensorConfig
{
    std::string name;
    std::string channel;
    double scale;
};

class SensorReader
{
public:
    SensorReader(Bus& bus, Journal& j, const Board& board, std::vector<SensorConfig> cfg)
        : bus_(bus), j_(j), board_(board), cfg_(std::move(cfg)) {}

    void poll()
    {
        for (const SensorConfig& c : cfg_) {
            int raw = 0;
            const std::string path = "/xyz/openbmc_project/sensors/temperature/" + c.name;
            if (!board_.read_raw(c.channel, raw)) {
                if (!warned_[c.name]) {
                    j_.log("sensor-reader", "ERROR " + c.name + ": channel '" + c.channel + "' not found; object not created");
                    warned_[c.name] = true;
                }
                continue;
            }
            bus_.set(path, "Value", raw * c.scale);
        }
    }

private:
    Bus& bus_;
    Journal& j_;
    const Board& board_;
    std::vector<SensorConfig> cfg_;
    std::map<std::string, bool> warned_;
};

// ---- daemon 2: threshold monitor: watches every temperature and logs crossings
class ThresholdMonitor
{
public:
    ThresholdMonitor(Bus& bus, Journal& j, double warning, double critical)
        : j_(j), warning_(warning), critical_(critical)
    {
        bus.watch([this](const std::string& path, const std::string& prop, double v) { changed(path, prop, v); });
    }

private:
    void changed(const std::string& path, const std::string& prop, double v)
    {
        if (prop != "Value" || path.find("/sensors/temperature/") == std::string::npos) {
            return;
        }
        const int level = v >= critical_ ? 2 : (v >= warning_ ? 1 : 0);
        int& old = level_[path];
        if (level != old) {
            static const char* names[] = {"normal", "WARNING high", "CRITICAL high"};
            std::ostringstream os;
            os << path.substr(path.rfind('/') + 1) << " " << names[old] << " -> " << names[level]
               << " at " << v << " C";
            j_.log("threshold-mon", os.str());
            old = level;
        }
    }

    Journal& j_;
    double warning_;
    double critical_;
    std::map<std::string, int> level_;
};

// ---- daemon 3: fan control: needs every expected sensor; otherwise fail safe at 100 %
class FanControl
{
public:
    FanControl(Bus& bus, Journal& j, std::vector<std::string> expected)
        : bus_(bus), j_(j), expected_(std::move(expected)) {}

    void step()
    {
        double hottest = -1000.0;
        std::string missing;
        for (const std::string& name : expected_) {
            const std::string path = "/xyz/openbmc_project/sensors/temperature/" + name;
            if (!bus_.has(path)) {
                missing += (missing.empty() ? "" : ",") + name;
                continue;
            }
            hottest = std::max(hottest, bus_.get(path, "Value"));
        }
        double pwm = 0.0;
        if (!missing.empty()) {
            pwm = 100.0;                            // cannot see a sensor: assume the worst
            if (!failsafe_) {
                j_.log("fan-control", "FAILSAFE: missing input(s) " + missing + "; all fans 100 %");
                failsafe_ = true;
            }
        } else {
            pwm = std::clamp(30.0 + (hottest - 40.0) * 3.5, 30.0, 100.0);   // the model's fan curve
        }
        bus_.set("/xyz/openbmc_project/control/fanpwm/fan0", "Target", std::round(pwm));
    }

private:
    Bus& bus_;
    Journal& j_;
    std::vector<std::string> expected_;
    bool failsafe_ = false;
};

// ---- daemon 4: web front end: answers a request by reading the bus only
inline void web_get_sensors(const Bus& bus, Journal& j)
{
    std::ostringstream os;
    os << "GET sensors ->";
    for (const auto& [path, props] : bus.objects()) {
        for (const auto& [prop, value] : props) {
            os << ' ' << path.substr(path.rfind('/') + 1) << '.' << prop << '=' << value;
        }
    }
    j.log("web-frontend", os.str());
}
