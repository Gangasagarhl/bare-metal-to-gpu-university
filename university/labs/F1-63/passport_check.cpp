// F1-63 Listing 1: a checker for a GPU "hardware passport".
// Each passport line: field | value | tag
//   tag DOC: <document title> §<section>    a number from the vendor's document
//   tag MEAS: <run record file>             a number you measured (label it as a measurement)
//   tag TOOL: <run record file>             a value printed by a tool (compiler, device query)
//   tag TBD                                 not filled in yet
// Rules checked (guide AH-21 to AH-23): every filled-in value has a DOC, MEAS or TOOL tag;
// DOC tags name a section; "peak"/"max"/"theoretical" fields are not tagged MEAS;
// "measured" fields are not tagged DOC. Lines starting with # are comments.
#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>

std::string trim(const std::string& s)
{
    std::size_t a = s.find_first_not_of(" \t");
    std::size_t b = s.find_last_not_of(" \t\r");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

bool hasDigit(const std::string& s)
{
    for (unsigned char c : s) {
        if (std::isdigit(c)) { return true; }
    }
    return false;
}

bool contains(const std::string& s, const char* word) { return s.find(word) != std::string::npos; }

int main()
{
    std::string line;
    int lineNo = 0, ok = 0, open = 0, problems = 0;
    while (std::getline(std::cin, line)) {
        ++lineNo;
        if (trim(line).empty() || trim(line)[0] == '#') { continue; }
        std::size_t p1 = line.find('|');
        std::size_t p2 = p1 == std::string::npos ? std::string::npos : line.find('|', p1 + 1);
        if (p2 == std::string::npos) {
            std::printf("line %2d  PROBLEM  not in the form field | value | tag\n", lineNo);
            ++problems;
            continue;
        }
        const std::string field = trim(line.substr(0, p1));
        const std::string value = trim(line.substr(p1 + 1, p2 - p1 - 1));
        const std::string tag = trim(line.substr(p2 + 1));
        std::string verdict = "ok";
        if (tag == "TBD") {
            verdict = value == "TBD" ? "open (TBD)" : "PROBLEM  value filled in, but its source is TBD";
        } else if (tag.rfind("DOC:", 0) != 0 && tag.rfind("MEAS:", 0) != 0 && tag.rfind("TOOL:", 0) != 0) {
            verdict = hasDigit(value) ? "PROBLEM  number without a DOC, MEAS or TOOL tag"
                                      : "PROBLEM  value without a DOC, MEAS or TOOL tag";
        } else if (tag.rfind("DOC:", 0) == 0 && !contains(tag, "\xC2\xA7")) {
            verdict = "PROBLEM  DOC tag names no section";
        } else if (tag.rfind("MEAS:", 0) == 0 &&
                   (contains(field, "peak") || contains(field, "max") || contains(field, "theoretical"))) {
            verdict = "PROBLEM  a measurement cannot be the product's peak (AH-23)";
        } else if (tag.rfind("DOC:", 0) == 0 && contains(field, "measured")) {
            verdict = "PROBLEM  a 'measured' field must cite a run, not a document";
        }
        if (verdict == "ok") { ++ok; } else if (verdict.rfind("open", 0) == 0) { ++open; } else { ++problems; }
        std::printf("line %2d  %-60s  %s = %s\n", lineNo, verdict.c_str(), field.c_str(), value.c_str());
    }
    std::printf("summary: %d ok, %d open, %d problems -> %s\n", ok, open, problems,
                problems == 0 ? (open == 0 ? "passport complete" : "passport consistent, not complete")
                              : "fix the problems before anyone relies on this passport");
    return 0;
}
