// claims.cpp - check that every claim in a defence is backed by recorded evidence (SE501, F12-30).
// Reads from standard input:
//   evidence <id> test <pass|fail> <file>
//   evidence <id> measurement <file> machine:<yes|no> date:<yes|no> runs:<n>
//   evidence <id> source <title-in-one-word> opened:<yes|no>
//   claim <id> <claim|limitation> <evidence ids, comma separated, or -> <text...>
// Prints one verdict per claim: BACKED, WEAK (with the reason), UNBACKED, or STATED for a
// limitation given without evidence (saying what was not tested needs no proof that it works).
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Evidence
{
    std::string kind, result, file;
    bool machine = false, date = false, opened = false;
    int runs = 0;
};

int main()
{
    std::map<std::string, Evidence> ev;
    int unbacked = 0, weak = 0, backed = 0;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        if (!(in >> kind) || kind[0] == '#') continue;
        if (kind == "evidence") {
            std::string id;
            Evidence e;
            in >> id >> e.kind;
            std::string tok;
            if (e.kind == "test") in >> e.result >> e.file;
            if (e.kind == "measurement") in >> e.file;
            if (e.kind == "source") in >> e.file;
            while (in >> tok) {
                if (tok == "machine:yes") e.machine = true;
                if (tok == "date:yes") e.date = true;
                if (tok == "opened:yes") e.opened = true;
                if (tok.rfind("runs:", 0) == 0) e.runs = std::stoi(tok.substr(5));
            }
            ev[id] = e;
            continue;
        }
        if (kind != "claim") continue;
        std::string id, type, refs, text, word;
        in >> id >> type >> refs;
        while (in >> word) text += (text.empty() ? "" : " ") + word;
        std::vector<std::string> problems;
        int usable = 0;
        std::stringstream rs(refs);
        std::string r;
        while (std::getline(rs, r, ',')) {
            if (r == "-") continue;
            auto it = ev.find(r);
            if (it == ev.end()) {
                problems.push_back(r + " is not in the evidence list");
                continue;
            }
            const Evidence& e = it->second;
            if (e.kind == "test" && e.result != "pass" && type == "claim") {
                problems.push_back(r + " is a failing test; state it as a limitation");
            } else if (e.kind == "measurement" && !(e.machine && e.date)) {
                problems.push_back(r + " has no machine or no date (AH-23)");
            } else if (e.kind == "measurement" && e.runs < 20) {
                problems.push_back(r + " has " + std::to_string(e.runs) +
                                   " runs; the plan said 20");
            } else if (e.kind == "source" && !e.opened) {
                problems.push_back(r + " was not opened; mark the claim unverified");
            } else {
                ++usable;
            }
        }
        std::string verdict = "BACKED";
        if (type == "limitation" && refs == "-") {
            verdict = "STATED";  // an honest limitation needs no evidence that it works
            ++backed;
        } else if (usable == 0) {
            verdict = "UNBACKED";
            ++unbacked;
        } else if (!problems.empty()) {
            verdict = "WEAK";
            ++weak;
        } else {
            ++backed;
        }
        std::cout << id << " [" << type << "] " << verdict << ": " << text << "\n";
        for (const auto& p : problems) std::cout << "    - " << p << "\n";
    }
    std::cout << "backed or stated " << backed << ", weak " << weak << ", unbacked " << unbacked
              << "\n";
    return unbacked == 0 ? 0 : 1;
}
