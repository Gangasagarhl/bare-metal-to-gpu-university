// study_check.cpp - DR405 F4-47: check feasibility-study drafts against the acceptance
// test of milestones G4 and G6: the study must list every firmware blob (or say "none" with
// a source), the register blocks, the documentation and its gaps, a stop criterion and a
// way to verify the result; every item names a source or says UNVERIFIED.
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
struct Study {
    std::string title;
    std::map<std::string, int> kinds;
    int items = 0, unverified = 0, no_source = 0;
};

std::string trim(const std::string& s)
{
    const size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
}  // namespace

int main()
{
    std::vector<Study> studies;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.rfind("=== study:", 0) == 0) {
            studies.push_back(Study{trim(line.substr(10)), {}, 0, 0, 0});
            continue;
        }
        if (studies.empty() || line.rfind("item ", 0) != 0) continue;
        Study& s = studies.back();
        const size_t bar = line.find('|');
        const std::string kind = trim(line.substr(5, bar == std::string::npos ? std::string::npos : bar - 5));
        ++s.kinds[kind];
        ++s.items;
        const size_t src = line.find("source:");
        if (src == std::string::npos) ++s.no_source;
        else if (line.find("UNVERIFIED", src) != std::string::npos) ++s.unverified;
    }
    int failing = 0;
    for (const Study& s : studies) {
        std::printf("%s\n  %d items, %d marked UNVERIFIED, %d without a source\n", s.title.c_str(), s.items, s.unverified,
                    s.no_source);
        std::vector<std::string> missing;
        for (const char* k : {"firmware", "register", "doc", "gap", "stop", "verify"})
            if (!s.kinds.count(k)) missing.push_back(k);
        for (const std::string& k : missing) std::printf("  MISSING: no '%s' item\n", k.c_str());
        const bool ok = missing.empty() && s.no_source == 0;
        std::printf("  verdict: %s\n\n", ok ? "complete enough to review (G4/G6 acceptance, study part)"
                                            : "not acceptable yet");
        failing += !ok;
    }
    std::printf("%d of %zu studies not acceptable yet\n", failing, studies.size());
    return 0;
}
