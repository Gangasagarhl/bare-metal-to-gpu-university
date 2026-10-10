// power_audit.cpp - evidence generator for the F5-29 forensic lab "Who switched off r07?".
// A deterministic simulation (the university's own, not a real BMC): two BMCs with audit logs,
// an inventory file that maps rack positions to BMC addresses, and an automation script that
// powers a server off by its rack position. The injected fault is written in the answer key.
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Bmc
{
    std::string address;
    std::string serial;          // the serial number of the server this BMC sits in
    std::string power = "On";
    std::vector<std::string> audit;

    std::string request(const std::string& clock, const std::string& from, const std::string& user,
                        const std::string& what)
    {
        audit.push_back(clock + " session from " + from + " user " + user + ": " + what);
        if (what == "ForceOff") {
            power = "Off";
            audit.push_back(clock + " host power state On -> Off (requested by " + user + ")");
        }
        return serial;
    }
};

std::string clock_at(int seconds)  // seconds after 23:00:00
{
    std::ostringstream os;
    os << "23:" << std::setw(2) << std::setfill('0') << seconds / 60 << ':'
       << std::setw(2) << std::setfill('0') << seconds % 60;
    return os.str();
}

int main()
{
    std::map<std::string, Bmc> bmcs{
        {"10.20.0.17", Bmc{"10.20.0.17", "R07-0001", "On", {}}},
        {"10.20.0.18", Bmc{"10.20.0.18", "R08-0001", "On", {}}},
    };
    // the inventory as the automation script reads it
    const std::vector<std::pair<std::string, std::string>> inventory{
        {"rack1/r07", "10.20.0.18"},
        {"rack1/r08", "10.20.0.17"},
    };
    std::vector<std::string> script_log;
    // 23:04 a read-only health poll of both positions (serials are logged but not compared)
    for (const auto& [position, address] : inventory) {
        const std::string serial = bmcs.at(address).request(clock_at(240), "10.20.9.5", "automation", "GET system");
        script_log.push_back(clock_at(240) + " poll " + position + " via " + address + ": ok, serial " + serial);
    }
    // 23:10 the night operator asks the script to power off r08 for a memory swap
    script_log.push_back(clock_at(600) + " job 'maint-r08' by operator Mei: power off rack1/r08");
    for (const auto& [position, address] : inventory) {
        if (position == "rack1/r08") {
            const std::string serial = bmcs.at(address).request(clock_at(601), "10.20.9.5", "automation", "ForceOff");
            script_log.push_back(clock_at(601) + " ForceOff sent to " + address + ": accepted, serial " + serial);
        }
    }
    script_log.push_back(clock_at(603) + " job 'maint-r08' finished: success");
    // the monitoring system's view of the two servers' services
    const std::vector<std::string> monitoring{
        clock_at(608) + " ALERT r07 web service: no response (3 checks)",
        clock_at(608) + " OK    r08 batch service: responding",
    };

    std::cout << "== evidence 1: automation script log (host 10.20.9.5)\n";
    for (const auto& l : script_log) { std::cout << l << '\n'; }
    std::cout << "\n== evidence 2: inventory file used by the script\n";
    for (const auto& [position, address] : inventory) { std::cout << position << "  bmc " << address << '\n'; }
    int number = 3;
    for (const auto& [address, bmc] : bmcs) {
        std::cout << "\n== evidence " << number++ << ": audit log of BMC " << address << " (power now " << bmc.power << ")\n";
        for (const auto& l : bmc.audit) { std::cout << l << '\n'; }
    }
    std::cout << "\n== evidence 5: monitoring\n";
    for (const auto& l : monitoring) { std::cout << l << '\n'; }
    return 0;
}
