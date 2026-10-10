// F9-15: a checker for a robot's wiring and power plan (shared by plan_check.cpp and
// plan_forensic.cpp). The plan is a text file; every line copies numbers from a datasheet
// and must end with the citation it came from (cite=...). Line types:
//   part   NAME supply_min supply_max supply_abs_max cite=...
//   rail   NAME volts max_current_A cite=...
//   power  PART RAIL current_A cite=...
//   signal NET DRIVER RECEIVER voh_min voh_max vol_max vih_min vil_max vin_abs_max cite=...
//   i2c    BUS PART address_hex cite=...
// The rules are the course's design rules; the 80 % rail rule is a team choice, not a law.
#pragma once
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Part
{
    double vmin = 0, vmax = 0, vabs = 0;
};
struct Rail
{
    double volts = 0, maxA = 0, loadA = 0;
};

inline int checkPlan(std::istream& in)
{
    std::map<std::string, Part> parts;
    std::map<std::string, Rail> rails;
    std::map<std::string, std::string> i2cUsers;  // "bus/address" -> part
    int errors = 0, warnings = 0, lineNo = 0;
    std::string line;
    auto report = [&](const char* level, const std::string& msg) {
        std::printf("line %2d  %-7s %s\n", lineNo, level, msg.c_str());
        if (std::string(level) == "WARN") ++warnings; else ++errors;
    };
    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty() || line[0] == '#') continue;
        if (line.find("cite=") == std::string::npos) report("WARN", "no datasheet citation");
        std::istringstream ss(line);
        std::string kind;
        ss >> kind;
        char buf[200];
        if (kind == "part") {
            std::string n;
            Part p;
            ss >> n >> p.vmin >> p.vmax >> p.vabs;
            parts[n] = p;
        } else if (kind == "rail") {
            std::string n;
            Rail r;
            ss >> n >> r.volts >> r.maxA;
            rails[n] = r;
        } else if (kind == "power") {
            std::string pn, rn;
            double a = 0;
            ss >> pn >> rn >> a;
            if (!parts.contains(pn) || !rails.contains(rn)) {
                report("ERROR", "unknown part or rail: " + pn + " / " + rn);
                continue;
            }
            const Part& p = parts[pn];
            Rail& r = rails[rn];
            r.loadA += a;
            if (r.volts > p.vabs) {
                std::snprintf(buf, sizeof buf, "%s on %s: %.2f V is ABOVE its absolute maximum "
                              "%.2f V (damage)", pn.c_str(), rn.c_str(), r.volts, p.vabs);
                report("DAMAGE", buf);
            } else if (r.volts < p.vmin || r.volts > p.vmax) {
                std::snprintf(buf, sizeof buf, "%s on %s: %.2f V is outside its operating range "
                              "%.2f..%.2f V", pn.c_str(), rn.c_str(), r.volts, p.vmin, p.vmax);
                report("ERROR", buf);
            }
        } else if (kind == "signal") {
            std::string net, drv, rcv;
            double vohMin = 0, vohMax = 0, volMax = 0, vihMin = 0, vilMax = 0, vinAbs = 0;
            ss >> net >> drv >> rcv >> vohMin >> vohMax >> volMax >> vihMin >> vilMax >> vinAbs;
            if (vohMax > vinAbs) {
                std::snprintf(buf, sizeof buf, "%s: %s can drive %.2f V into %s, above its input "
                              "absolute maximum %.2f V (damage)", net.c_str(), drv.c_str(), vohMax,
                              rcv.c_str(), vinAbs);
                report("DAMAGE", buf);
            }
            if (vohMin < vihMin) {
                std::snprintf(buf, sizeof buf, "%s: high level not guaranteed: VOH(min) %.2f V < "
                              "VIH(min) %.2f V of %s", net.c_str(), vohMin, vihMin, rcv.c_str());
                report("ERROR", buf);
            }
            if (volMax > vilMax) {
                std::snprintf(buf, sizeof buf, "%s: low level not guaranteed: VOL(max) %.2f V > "
                              "VIL(max) %.2f V of %s", net.c_str(), volMax, vilMax, rcv.c_str());
                report("ERROR", buf);
            }
        } else if (kind == "i2c") {
            std::string bus, pn, addr;
            ss >> bus >> pn >> addr;
            const std::string key = bus + "/" + addr;
            if (i2cUsers.contains(key)) {
                report("ERROR", "I2C address " + addr + " on " + bus + " used by both " +
                                    i2cUsers[key] + " and " + pn);
            } else {
                i2cUsers[key] = pn;
            }
        } else {
            report("ERROR", "unknown line type " + kind);
        }
    }
    for (const auto& [n, r] : rails) {
        char buf[200];
        std::snprintf(buf, sizeof buf, "rail %s: load %.2f A of %.2f A (%.0f %%)", n.c_str(),
                      r.loadA, r.maxA, 100.0 * r.loadA / r.maxA);
        if (r.loadA > 0.8 * r.maxA) {
            std::printf("rail     %-7s %s, above the 80 %% design rule\n", "WARN", buf);
            ++warnings;
        } else {
            std::printf("rail     %-7s %s\n", "ok", buf);
        }
    }
    std::printf("%d error(s), %d warning(s)\n", errors, warnings);
    return errors;
}
