// doc_lint.cpp - SE301 F12-02: mechanical checks on a design document (read from stdin).
// It finds what a machine can find: missing sections, goals with no acceptance test,
// vague words, and numbers without units in the Interfaces and Failure modes sections.
// It cannot tell whether the design is GOOD; that is what the human review is for.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

static std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static std::vector<std::string> words(const std::string& line)
{
    std::vector<std::string> out;
    std::string w;
    for (char c : line + " ") {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_') {
            w += c;
        } else if (!w.empty()) {
            while (!w.empty() && w.back() == '.') w.pop_back();   // sentence full stop
            if (!w.empty()) out.push_back(w);
            w.clear();
        }
    }
    return out;
}

static bool isNumber(const std::string& w)
{
    if (w.rfind("0x", 0) == 0) return false;                   // register values have no unit
    return !w.empty() && std::all_of(w.begin(), w.end(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c)) || c == '.';
    });
}

int main()
{
    const std::vector<std::string> required = {"Context", "Goals", "Non-goals", "Design",
        "Interfaces", "Alternatives considered", "Failure modes", "Test plan", "Open questions"};
    const std::set<std::string> vague = {"fast", "quickly", "robust", "simple", "easy",
        "efficient", "etc", "tbd", "appropriate", "appropriately", "reasonable", "properly",
        "soon"};
    const std::set<std::string> units = {"ns", "us", "ms", "s", "byte", "bytes", "bit", "bits",
        "kib", "mib", "hz", "khz", "baud", "%", "slots", "entries", "times", "lines"};

    std::set<std::string> seen;
    std::map<std::string, int> goals;            // goal id -> line
    std::set<std::string> tested;                // goal ids named by a test line
    std::string section;
    std::string line;
    int n = 0;
    int findings = 0;
    while (std::getline(std::cin, line)) {
        ++n;
        if (line.rfind("## ", 0) == 0) {
            section = line.substr(3);
            seen.insert(section);
            continue;
        }
        const std::vector<std::string> ws = words(line);
        if (section == "Goals" && !ws.empty() && ws[0].size() >= 2 && ws[0][0] == 'G') {
            goals[ws[0]] = n;
        }
        if (section == "Test plan") {
            for (const auto& w : ws) {
                const bool goalId = w.size() >= 2 && w[0] == 'G' &&
                                    std::isdigit(static_cast<unsigned char>(w[1]));
                if (goalId) {
                    tested.insert(w);
                }
            }
        }
        for (std::size_t i = 0; i < ws.size(); ++i) {
            if (vague.count(lower(ws[i]))) {
                std::printf("line %3d  vague word \"%s\": replace it with a number or a test\n",
                            n, ws[i].c_str());
                ++findings;
            }
            if ((section == "Interfaces" || section == "Failure modes") && isNumber(ws[i])) {
                const bool hasUnit = i + 1 < ws.size() && units.count(lower(ws[i + 1]));
                if (!hasUnit) {
                    std::printf("line %3d  number \"%s\" has no unit\n", n, ws[i].c_str());
                    ++findings;
                }
            }
        }
    }
    for (const auto& s : required) {
        if (!seen.count(s)) {
            std::printf("missing section \"## %s\"\n", s.c_str());
            ++findings;
        }
    }
    for (const auto& [id, at] : goals) {
        if (!tested.count(id)) {
            std::printf("goal %s (line %d) has no acceptance test in \"## Test plan\"\n",
                        id.c_str(), at);
            ++findings;
        }
    }
    std::printf("%d lines, %zu goals, %d findings\n", n, goals.size(), findings);
    return findings == 0 ? 0 : 1;
}
