// MP3 Listing 3: check the R1 targets file before any kernel is written, and print its
// fingerprint. The reviewer writes the fingerprint into the R1 gate record; at R2 and R4
// the report (Listing 4) prints the fingerprint of the targets it used, and they must match.
#include "mp3_targets.hpp"
#include <cstdlib>
#include <iostream>
#include <set>

bool isPlaceholder(const std::vector<std::string>& w)
{
    if (w.empty()) {
        return true;
    }
    for (const std::string& s : w) {
        if (s.find('?') != std::string::npos || s.find('<') != std::string::npos) {
            return true;
        }
    }
    return false;
}

int main()
{
    const std::vector<Line> lines = parseRecords(std::cin);
    std::vector<std::string> problems;
    std::set<std::string> have, sgemmSizes;
    std::string author;
    int signatures = 0;
    for (const Line& l : lines) {
        if (isPlaceholder(l.words)) {                 // present but not filled: say so once
            problems.push_back("'" + l.kind + " " + joined(l.words) + "' is still a placeholder");
            const bool keyed = l.kind == "reference" || l.kind == "target" || l.kind == "tolerance";
            have.insert(keyed && !l.words.empty() ? l.kind + " " + l.words[0] : l.kind);
            if (l.kind == "target" && l.words.size() > 1 && l.words[0] == "sgemm") {
                sgemmSizes.insert(l.words[1]);
            }
            continue;
        }
        if (l.kind == "reference" || l.kind == "tolerance") {
            have.insert(l.kind + " " + l.words[0]);
        } else if (l.kind == "target") {
            if (l.words.size() != 4) {
                problems.push_back("target needs: kernel size precision percent");
                continue;
            }
            const double pct = std::atof(l.words[3].c_str());
            if (!(pct > 0.0 && pct <= 100.0)) {
                problems.push_back("target " + joined(l.words) + ": percent must be in (0, 100]");
            }
            have.insert("target " + l.words[0]);
            if (l.words[0] == "sgemm") {
                sgemmSizes.insert(l.words[1]);
            }
        } else if (l.kind == "author") {
            author = l.words[0];
        } else if (l.kind == "signed" && l.words.size() >= 3 && l.words[0] == "R1") {
            if (l.words[1] == author) {
                problems.push_back("R1 signed by the author '" + author + "': not independent");
            } else {
                ++signatures;
            }
        } else {
            have.insert(l.kind);
        }
    }
    for (const char* need : {"gpu", "driver", "toolkit", "clocks", "reference sgemm",
                             "reference hgemm", "reference attention", "target sgemm",
                             "target hgemm", "target attention", "tolerance fp32",
                             "tolerance fp16"}) {
        if (!have.count(need)) {
            problems.push_back(std::string("missing: ") + need);
        }
    }
    if (sgemmSizes.size() < 3 || !sgemmSizes.count("4096")) {
        problems.push_back("SGEMM needs three sizes, one of them 4096 (curriculum E6)");
    }
    if (signatures == 0) {
        problems.push_back("no independent R1 signature");
    }
    std::printf("targets in this file:\n");
    for (const Line& l : lines) {
        if (l.kind == "target") {
            std::printf("  %s\n", joined(l.words).c_str());
        }
    }
    for (const std::string& p : problems) {
        std::printf("PROBLEM: %s\n", p.c_str());
    }
    std::printf("fingerprint of the agreement: %s\n", hex64(fingerprint(lines)).c_str());
    std::printf("%s\n", problems.empty()
                           ? "R1 TARGETS READY: record the fingerprint in the gate record"
                           : "R1 TARGETS NOT READY: fix every PROBLEM, then ask for signatures");
    return problems.empty() ? 0 : 1;
}
