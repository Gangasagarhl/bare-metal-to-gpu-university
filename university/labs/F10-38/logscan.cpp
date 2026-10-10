// logscan.cpp - a small flight-log analyser for the course log format (CSV with '#' lines).
// For each log it prints: the parameters and events, a per-20-s table, and two checks:
//  1. navigation consistency: the angle from the commanded horizontal acceleration to the
//     acceleration actually measured from GNSS velocity; a steady offset means the vehicle
//     is not pushing where the software thinks it is (a heading error);
//  2. energy: charge used, the configured capacity, the loaded voltage per cell, and the
//     charge a return home would need from the current position (formula of F10-35).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Row
{
    std::map<std::string, double> v;
    std::string mode;
};

double param(const std::string& line, const std::string& key)
{
    const auto p = line.find(key + "=");
    return p == std::string::npos ? -1 : std::stod(line.substr(p + key.size() + 1));
}

double median(std::vector<double> x)
{
    if (x.empty()) return NAN;
    std::sort(x.begin(), x.end());
    return x[x.size() / 2];
}

void analyse(const std::string& file)
{
    std::ifstream in(file);
    std::string line;
    std::vector<std::string> cols;
    std::vector<Row> rows;
    double capacity = -1, cells = 4;
    std::printf("==================== %s\n", file.c_str());
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') {
            if (line.find("params:") != std::string::npos) {
                capacity = param(line, "batt_capacity_mah");
                cells = param(line, "cells");
            }
            std::printf("%s\n", line.c_str());
            continue;
        }
        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> f;
        while (std::getline(ss, cell, ',')) f.push_back(cell);
        if (cols.empty()) {
            cols = f;
            continue;
        }
        Row r;
        for (std::size_t i = 0; i < f.size() && i < cols.size(); ++i) {
            if (cols[i] == "mode")
                r.mode = f[i];
            else
                r.v[cols[i]] = std::stod(f[i]);
        }
        rows.push_back(r);
    }
    // Per-row derived values.
    std::vector<double> angle(rows.size(), NAN), norm;
    for (std::size_t i = 0; i + 1 < rows.size(); ++i) {
        auto& a = rows[i].v;
        auto& b = rows[i + 1].v;
        const double dt = b["t"] - a["t"];
        const double mx = (b["vx"] - a["vx"]) / dt, my = (b["vy"] - a["vy"]) / dt;
        const double cx = a["acx_cmd"], cy = a["acy_cmd"];
        if (std::hypot(cx, cy) > 1.0 && std::hypot(mx, my) > 0.5)
            angle[i] = std::atan2(cx * my - cy * mx, cx * mx + cy * my) * 180.0 / 3.14159265358979;
        if (rows[i].v["t"] > 7.0) norm.push_back(a["mag_norm"]);
    }
    std::printf("%6s %-7s %7s %7s %8s %7s %10s %9s %9s %8s %9s\n", "t[s]", "mode", "home[m]",
                "amps", "V/cell", "magnrm", "cmd->meas", "used", "rem[%]", "need", "spare");
    std::printf("%6s %-7s %7s %7s %8s %7s %10s %9s %9s %8s %9s\n", "", "", "", "(mean)", "(min)",
                "(mean)", "[deg,med]", "[mAh]", "", "[mAh]", "[mAh]");
    for (double t0 = 0; t0 < rows.back().v["t"] + 0.1; t0 += 20.0) {
        std::vector<double> ang;
        double sa = 0, sn = 0, vmin = 1e9, n = 0, dist = 0;
        const Row* last = nullptr;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const auto& r = rows[i].v;
            if (r.at("t") < t0 || r.at("t") >= t0 + 20.0) continue;
            if (!std::isnan(angle[i])) ang.push_back(angle[i]);
            sa += r.at("amps");
            sn += r.at("mag_norm");
            vmin = std::min(vmin, r.at("volts") / cells);
            ++n;
            dist = std::hypot(r.at("x"), r.at("y"));
            last = &rows[i];
        }
        if (!last) continue;
        // Return charge (F10-35 reserve.cpp formula, same constants, 30 % margin).
        const double t_back = 2.5 + dist / 5.0 + 15.0 / 0.7;
        const double need = t_back * (sa / n) / 3.6 * 1.3;
        const double spare = capacity - last->v.at("used_mah");
        std::printf("%6.0f %-7s %7.1f %7.1f %8.2f %7.3f %10.0f %9.0f %9.1f %8.0f %9.0f\n", t0,
                    last->mode.c_str(), dist, sa / n, vmin, sn / n, median(ang),
                    last->v.at("used_mah"), last->v.at("rem_pct"), need, spare);
    }
    const auto [lo, hi] = std::minmax_element(norm.begin(), norm.end());
    std::printf("mag_norm: on the ground before arming %.3f; in flight min %.3f, max %.3f\n",
                rows.front().v["mag_norm"], *lo, *hi);
    std::printf("charge used at end of log: %.0f mAh; configured capacity: %.0f mAh\n",
                rows.back().v["used_mah"], capacity);
}

int main()
{
    analyse("log_a_flyaway.csv");
    analyse("log_b_late_battery.csv");
    return 0;
}
