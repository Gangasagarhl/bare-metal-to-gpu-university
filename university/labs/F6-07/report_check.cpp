// F6-07 Listing 2: compare our device report with a vendor tool's numbers, key by key
// (milestone E1: SM count, memory size, warp size, compute capability must match).
// Input: lines "ours key=value" and "vendor key=value". The input file of this build
// uses invented example values (no GPU was available).
#include <cstdio>
#include <iostream>
#include <map>
#include <string>

int main()
{
    std::map<std::string, std::string> ours;
    std::map<std::string, std::string> vendor;
    std::string who, kv;
    while (std::cin >> who >> kv) {
        const auto eq = kv.find('=');
        if (eq == std::string::npos) { continue; }
        (who == "ours" ? ours : vendor)[kv.substr(0, eq)] = kv.substr(eq + 1);
    }
    const char* required[] = {"sm_count", "memory_bytes", "warp_size", "compute_capability"};
    int failures = 0;
    for (const char* key : required) {
        const auto a = ours.find(key);
        const auto b = vendor.find(key);
        if (a == ours.end() || b == vendor.end()) {
            std::printf("%-20s MISSING (ours: %s, vendor: %s)\n", key,
                        a == ours.end() ? "-" : a->second.c_str(),
                        b == vendor.end() ? "-" : b->second.c_str());
            ++failures;
        } else if (a->second != b->second) {
            std::printf("%-20s MISMATCH ours=%s vendor=%s\n", key, a->second.c_str(),
                        b->second.c_str());
            ++failures;
        } else {
            std::printf("%-20s ok (%s)\n", key, a->second.c_str());
        }
    }
    std::printf("E1 device-report test: %s\n", failures == 0 ? "PASS" : "FAIL");
    return 0;
}
