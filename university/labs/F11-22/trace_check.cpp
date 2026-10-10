// trace_check.cpp - F11-22: check traceability from safety goals to test evidence.
// Input lines (fields separated by '|'):
//   RELEASE|<build id>                                the software build being released
//   R|<id>|<revision>|<parent id or ->|<text>         safety goal (parent -) or requirement
//   D|<id>|<requirement id>|<text>                     design element implementing a requirement
//   T|<id>|<requirement id>|<revision verified>|<text> test case verifying a requirement revision
//   X|<test id>|<PASS or FAIL>|<build id>|<evidence>   a recorded test execution
// Checks: goals refined into requirements; every leaf requirement implemented and verified;
// tests verify the current revision; every test has a passing result on the release build;
// no orphan tests.
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Req {
    std::string id, parent, text;
    int rev = 0;
};
struct TestCase {
    std::string id, req, text;
    int rev = 0;
};
struct Exec {
    std::string verdict, build, evidence;
};

std::vector<std::string> fields(const std::string& line)
{
    std::vector<std::string> f;
    std::string item;
    std::istringstream in(line);
    while (std::getline(in, item, '|')) {
        f.push_back(item);
    }
    return f;
}

int main()
{
    std::string release, line;
    std::vector<Req> reqs;
    std::multimap<std::string, std::string> design;    // requirement -> design element
    std::vector<TestCase> tests;
    std::map<std::string, Exec> results;               // test -> latest execution
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto f = fields(line);
        if (f[0] == "RELEASE") {
            release = f.at(1);
        } else if (f[0] == "R") {
            reqs.push_back({f.at(1), f.at(3), f.at(4), std::stoi(f.at(2))});
        } else if (f[0] == "D") {
            design.insert({f.at(2), f.at(1)});
        } else if (f[0] == "T") {
            tests.push_back({f.at(1), f.at(2), f.at(4), std::stoi(f.at(3))});
        } else if (f[0] == "X") {
            results[f.at(1)] = {f.at(2), f.at(3), f.at(4)};
        }
    }
    std::map<std::string, const Req*> by_id;
    std::map<std::string, int> children;
    for (const auto& r : reqs) {
        by_id[r.id] = &r;
        if (r.parent != "-") {
            ++children[r.parent];
        }
    }
    int gaps = 0;
    auto gap = [&gaps](const std::string& msg) {
        std::printf("  GAP  %s\n", msg.c_str());
        ++gaps;
    };
    std::printf("Release build %s\n\nTrace matrix (requirement -> design -> tests -> result):\n",
                release.c_str());
    for (const auto& r : reqs) {
        std::string d, t;
        const auto range = design.equal_range(r.id);
        for (auto it = range.first; it != range.second; ++it) {
            d += (d.empty() ? "" : ",") + it->second;
        }
        for (const auto& tc : tests) {
            if (tc.req == r.id) {
                const auto res = results.find(tc.id);
                const std::string st = res == results.end() ? "no result"
                                       : res->second.verdict + "@" + res->second.build;
                t += (t.empty() ? "" : "; ") + tc.id + " " + st;
            }
        }
        std::printf("  %-4s r%d %-4s %-8s %-6s %s\n", r.id.c_str(), r.rev, r.parent.c_str(),
                    d.empty() ? "-" : d.c_str(), children[r.id] > 0 ? "(refined)" : "",
                    t.c_str());
    }
    std::puts("\nChecks:");
    for (const auto& r : reqs) {
        if (r.parent == "-" && children[r.id] == 0) {
            gap(r.id + " (safety goal) is not refined into any requirement");
        }
        if (r.parent != "-" && children[r.id] == 0) {
            if (design.count(r.id) == 0) {
                gap(r.id + " has no design element implementing it");
            }
            bool verified = false;
            for (const auto& tc : tests) {
                verified = verified || tc.req == r.id;
            }
            if (!verified) {
                gap(r.id + " has no test");
            }
        }
    }
    for (const auto& tc : tests) {
        const auto r = by_id.find(tc.req);
        if (r == by_id.end()) {
            gap(tc.id + " traces to " + tc.req + ", which does not exist (orphan test)");
            continue;
        }
        if (tc.rev != r->second->rev) {
            gap(tc.id + " verifies " + tc.req + " r" + std::to_string(tc.rev) +
                " but the requirement is now r" + std::to_string(r->second->rev));
        }
        const auto res = results.find(tc.id);
        if (res == results.end()) {
            gap(tc.id + " (" + tc.text + ") has no recorded result");
        } else if (res->second.build != release) {
            gap(tc.id + " result is from build " + res->second.build + ", not the release");
        } else if (res->second.verdict != "PASS") {
            gap(tc.id + " FAILED on the release build (" + res->second.evidence + ")");
        }
    }
    if (gaps == 0) {
        std::puts("  (none)");
    }
    std::printf("\n%zu requirements, %zu tests, %d gaps: %s\n", reqs.size(), tests.size(), gaps,
                gaps == 0 ? "evidence complete for this release"
                          : "evidence NOT complete for this release");
    return 0;
}
