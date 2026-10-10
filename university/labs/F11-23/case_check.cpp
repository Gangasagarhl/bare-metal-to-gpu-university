// case_check.cpp - F11-23: check the structure and the evidence of a safety case.
// Input lines (fields separated by '|'), a small text form of a goal-structured argument:
//   G|<id>|<parent or ->|<scope: sim, hw or any>|<claim>         goal (a claim)
//   S|<id>|<parent goal>|<argument strategy>                     strategy (how a goal is argued)
//   E|<id>|<parent goal>|<kind: sim, analysis, review or hw>|<file>|<texts that must appear;...>
//   C|<id>|<parent>|<context>     A|<id>|<parent>|<assumption>   context and assumptions
//   U|<goal id>|<why it is open>                                 goal declared undeveloped
// Evidence files are read for real (paths relative to this lab folder). A goal is supported when
// all its children are; evidence is supported when the file exists and shows every listed text.
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Node {
    char type = '?';
    std::string id, parent, scope, text, file, must;
};

std::vector<Node> nodes;
std::map<std::string, std::string> open_goals;    // id -> reason
int defects = 0, open_count = 0;

std::vector<std::string> split(const std::string& s, char sep)
{
    std::vector<std::string> f;
    std::string item;
    std::istringstream in(s);
    while (std::getline(in, item, sep)) {
        f.push_back(item);
    }
    return f;
}

std::string evidence_status(const Node& e)
{
    std::ifstream in(e.file);
    if (!in) {
        return "MISSING (no file " + e.file + ")";
    }
    std::stringstream buf;
    buf << in.rdbuf();
    for (const auto& text : split(e.must, ';')) {
        if (buf.str().find(text) == std::string::npos) {
            return "DOES NOT SHOW \"" + text + "\"";
        }
    }
    return "ok";
}

bool check(const Node& n, int depth, const std::string& scope)
{
    std::string status;
    bool supported = true;
    if (n.type == 'E') {
        status = evidence_status(n);
        if (scope == "hw" && n.scope != "hw") {
            const std::string why = "SCOPE: " + n.scope + " evidence for a hardware claim";
            status = status == "ok" ? why : status + "; " + why;
        }
        supported = status == "ok";
    }
    std::printf("%*s%c %-4s %s%s%s\n", depth * 2, "", n.type, n.id.c_str(), n.text.c_str(),
                n.type == 'E' ? "  [" : "", n.type == 'E' ? (status + "]").c_str() : "");
    if (n.type == 'E') {
        defects += supported ? 0 : 1;
        return supported;
    }
    if (n.type == 'C' || n.type == 'A') {
        return true;
    }
    int children = 0;
    for (const auto& c : nodes) {
        if (c.parent == n.id) {
            const bool counts = c.type != 'C' && c.type != 'A';
            const bool ok = check(c, depth + 1, n.type == 'G' ? n.scope : scope);
            children += counts ? 1 : 0;
            supported = supported && ok;
        }
    }
    if (n.type == 'G' && open_goals.count(n.id) != 0) {
        std::printf("%*s  OPEN: %s\n", depth * 2, "", open_goals[n.id].c_str());
        ++open_count;
        return false;
    }
    if (children == 0) {
        std::printf("%*s  DEFECT: nothing supports this %s\n", depth * 2, "",
                    n.type == 'G' ? "goal" : "strategy");
        ++defects;
        return false;
    }
    return supported;
}

int main()
{
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto f = split(line, '|');
        Node n;
        n.type = f.at(0)[0];
        n.id = f.at(1);
        if (n.type == 'U') {
            open_goals[n.id] = f.at(2);
            continue;
        }
        n.parent = f.at(2);
        if (n.type == 'G') {
            n.scope = f.at(3);
            n.text = f.at(4) + " (scope: " + n.scope + ")";
        } else if (n.type == 'E') {
            n.scope = f.at(3);
            n.file = f.at(4);
            n.must = f.at(5);
            n.text = n.scope + " evidence " + n.file;
        } else {
            n.text = (n.type == 'C' ? "context: " : n.type == 'A' ? "assumption: " : "") + f.at(3);
        }
        nodes.push_back(n);
    }
    bool top_ok = true;
    for (const auto& n : nodes) {
        if (n.parent == "-") {
            top_ok = check(n, 0, n.scope) && top_ok;
        }
    }
    std::printf("\nopen goals %d, defects %d: %s\n", open_count, defects,
                defects > 0 ? "the case is NOT acceptable as submitted"
                : open_count > 0 ? "the argument is sound so far; open goals must be closed"
                                   " before the claim holds"
                : (top_ok ? "every goal is supported by evidence" : "check the output"));
    return 0;
}
