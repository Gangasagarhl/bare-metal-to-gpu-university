// power_readout.cpp - what this machine says about its own power management, read only.
// Part A: CPUID leaf 6 (thermal and power management leaf), bit meanings written from memory of
//         the Intel SDM (title only in this build), each cross-checked against the flag name
//         the Linux kernel prints in /proc/cpuinfo for the same bit.
// Part B: the idle (cpuidle), frequency (cpufreq) and thermal folders in sysfs: what exists.
#include <cpuid.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

static std::string slurp(const std::string& p)
{
    std::ifstream f(p);
    if (!f) return "(absent)";
    std::string s, line;
    while (std::getline(f, line)) s += (s.empty() ? "" : " | ") + line;
    return s.empty() ? "(empty)" : s;
}
static int count_entries(const std::string& dir, const std::string& prefix)
{
    DIR* d = opendir(dir.c_str());
    if (!d) return -1;
    int n = 0;
    while (dirent* e = readdir(d)) {
        const std::string name = e->d_name;
        if (name != "." && name != ".." && name.rfind(prefix, 0) == 0) ++n;
    }
    closedir(d);
    return n;
}

int main()
{
    std::set<std::string> flags;
    {
        std::ifstream f("/proc/cpuinfo");
        std::string line;
        while (std::getline(f, line))
            if (line.rfind("flags", 0) == 0) {
                std::istringstream s(line.substr(line.find(':') + 1));
                for (std::string w; s >> w;) flags.insert(w);
                break;
            }
    }
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (!__get_cpuid_count(6, 0, &a, &b, &c, &d)) { std::cout << "CPUID leaf 6 not available\n"; return 1; }
    std::cout << "== A. CPUID leaf 6: eax 0x" << std::hex << a << " ecx 0x" << c << std::dec << " ==\n";
    struct Bit { char reg; int bit; const char* meaning; const char* flag; };
    const Bit bits[] = {
        {'a', 0, "digital temperature sensor", "dts"},
        {'a', 1, "turbo / dynamic acceleration", "ida"},
        {'a', 2, "APIC timer runs in every idle state (ARAT)", "arat"},
        {'a', 4, "power limit notification", "pln"},
        {'a', 6, "package thermal management", "pts"},
        {'a', 7, "hardware-controlled performance states (HWP)", "hwp"},
        {'c', 0, "APERF/MPERF counters", "aperfmperf"},
        {'c', 3, "energy/performance bias", "epb"},
    };
    int agree = 0, disagree = 0;
    for (const Bit& x : bits) {
        const unsigned reg = x.reg == 'a' ? a : c;
        const bool set = (reg >> x.bit) & 1;
        const bool listed = flags.count(x.flag) != 0;
        (set == listed ? agree : disagree)++;
        std::cout << "  e" << x.reg << "x bit " << x.bit << " " << (set ? "1" : "0") << "  "
                  << x.meaning << "; /proc/cpuinfo '" << x.flag << "' " << (listed ? "listed" : "not listed")
                  << (set == listed ? "" : "   <- DIFFERS") << "\n";
    }
    std::cout << "  agreement: " << agree << " of " << agree + disagree << " bits\n";

    std::cout << "== B. sysfs ==\n";
    const std::string ci = "/sys/devices/system/cpu/cpuidle/";
    std::cout << "  cpuidle current_driver:       " << slurp(ci + "current_driver") << "\n";
    std::cout << "  cpuidle current_governor:     " << slurp(ci + "current_governor") << "\n";
    std::cout << "  cpuidle available_governors:  " << slurp(ci + "available_governors") << "\n";
    std::cout << "  cpu0 idle states:             " << count_entries("/sys/devices/system/cpu/cpu0/cpuidle", "state") << "\n";
    std::cout << "  cpu0 cpufreq entries:         " << count_entries("/sys/devices/system/cpu/cpu0/cpufreq", "") << "\n";
    std::cout << "  thermal zones:                " << count_entries("/sys/class/thermal", "thermal_zone") << "\n";
    std::cout << "  cooling devices:              " << count_entries("/sys/class/thermal", "cooling_device") << "\n";
    std::cout << "(-1 = the folder does not exist on this machine)\n";
    return disagree ? 1 : 0;
}
