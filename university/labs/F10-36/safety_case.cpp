// safety_case.cpp - a checker for safety cases written in this course's small text format
// (claims, strategies, evidence, hazards, regulatory questions). It does not judge whether
// an argument is convincing - people do that in review. It finds the mechanical gaps:
// undeveloped claims, evidence that failed or belongs to another software version,
// hazards nobody argued about, risk above the team's own limit, rules without a source.
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Node
{
    std::string kind, id, parent, text, hazard, result, firmware, config;
};
struct Hazard
{
    std::string id, text;
    int sev = 0, like_before = 0, like_after = 0;
};
struct RegQ
{
    std::string id, question, answer, source, date;
};

std::vector<std::string> tokens(const std::string& line)  // words and "quoted strings"
{
    std::vector<std::string> t;
    std::size_t i = 0;
    while (i < line.size()) {
        if (line[i] == ' ' || line[i] == '\t') {
            ++i;
            continue;
        }
        if (line[i] == '"') {
            const std::size_t j = line.find('"', i + 1);
            t.push_back(line.substr(i + 1, j - i - 1));
            i = (j == std::string::npos) ? line.size() : j + 1;
        } else {
            std::size_t j = i;
            while (j < line.size() && line[j] != ' ' && line[j] != '\t') ++j;
            t.push_back(line.substr(i, j - i));
            i = j;
        }
    }
    return t;
}

std::string field(const std::vector<std::string>& t, const std::string& key)
{
    for (std::size_t i = 0; i + 1 < t.size(); ++i)
        if (t[i] == key) return t[i + 1];
    return "";
}

int main()
{
    std::map<std::string, Node> nodes;
    std::vector<std::string> order;
    std::vector<Hazard> hazards;
    std::vector<RegQ> regs;
    std::string fw, cfg;
    int accept = 0;
    std::string line;
    while (std::getline(std::cin, line)) {
        const auto t = tokens(line);
        if (t.empty() || t[0][0] == '#') continue;
        if (t[0] == "system") {
            fw = field(t, "firmware");
            cfg = field(t, "config");
        } else if (t[0] == "accept")
            accept = std::stoi(t[1]);
        else if (t[0] == "hazard")
            hazards.push_back({t[1], t[2], std::stoi(field(t, "sev")),
                               std::stoi(field(t, "before")), std::stoi(field(t, "after"))});
        else if (t[0] == "reg")
            regs.push_back({t[1], t[2], field(t, "answer"), field(t, "source"), field(t, "read")});
        else {  // goal | strategy | evidence  ID PARENT "text" [key value ...]
            Node n{t[0],
                   t[1],
                   t[2],
                   t[3],
                   field(t, "hazard"),
                   field(t, "result"),
                   field(t, "firmware"),
                   field(t, "config")};
            nodes[n.id] = n;
            order.push_back(n.id);
        }
    }

    std::map<std::string, int> children;
    for (const auto& id : order) children[nodes[id].parent]++;
    std::vector<std::string> findings;

    // Print the tree (depth-first from the top goal, whose parent is "-").
    std::printf("safety case for firmware %s, configuration %s\n", fw.c_str(), cfg.c_str());
    std::vector<std::pair<std::string, int>> stack;
    for (auto it = order.rbegin(); it != order.rend(); ++it)
        if (nodes[*it].parent == "-") stack.push_back({*it, 0});
    while (!stack.empty()) {
        const auto [id, depth] = stack.back();
        stack.pop_back();
        const Node& n = nodes[id];
        std::string mark;
        if (n.kind == "goal" && children[id] == 0) mark = "  <- UNDEVELOPED";
        if (n.kind == "evidence")
            mark = "  [" + n.result + ", " + n.firmware + "/" + n.config + "]";
        std::printf("%*s%s %s: %s%s\n", depth * 2, "", n.kind.c_str(), id.c_str(), n.text.c_str(),
                    mark.c_str());
        for (auto it = order.rbegin(); it != order.rend(); ++it)
            if (nodes[*it].parent == id) stack.push_back({*it, depth + 1});
    }

    // Rule 1: every reference resolves; every goal is developed.
    for (const auto& id : order) {
        const Node& n = nodes[id];
        if (n.parent != "-" && !nodes.count(n.parent))
            findings.push_back(id + ": parent " + n.parent + " does not exist");
        if (n.kind == "goal" && children[id] == 0)
            findings.push_back(id + ": claim has no argument or evidence below it");
        if (n.kind == "evidence" && n.result != "PASS")
            findings.push_back(id + ": evidence result is " + n.result);
        if (n.kind == "evidence" && (n.firmware != fw || n.config != cfg))
            findings.push_back(id + ": evidence is for " + n.firmware + "/" + n.config +
                               ", the case is for " + fw + "/" + cfg);
    }
    // Rule 2: every hazard is argued about, and its residual risk is within the team's limit.
    std::printf("\nhazards (risk = severity x likelihood; team limit %d)\n", accept);
    for (const Hazard& h : hazards) {
        bool argued = false;
        for (const auto& id : order) argued = argued || nodes[id].hazard == h.id;
        const int before = h.sev * h.like_before, after = h.sev * h.like_after;
        std::printf("  %s %-58s before %2d after %2d %s\n", h.id.c_str(), h.text.c_str(), before,
                    after, after <= accept ? "ok" : "ABOVE LIMIT");
        if (!argued) findings.push_back(h.id + ": no claim in the case addresses this hazard");
        if (after > accept)
            findings.push_back(h.id + ": residual risk " + std::to_string(after) + " above limit");
    }
    // Rule 3: every regulatory question has an answer, a named official source and a date read.
    std::printf("\nregulatory questions\n");
    const std::set<std::string> not_sources = {"", "memory", "forum", "video", "friend"};
    for (const RegQ& r : regs) {
        std::printf("  %s %s\n     answer: %s\n     source: %s (read %s)\n", r.id.c_str(),
                    r.question.c_str(), r.answer.empty() ? "(none)" : r.answer.c_str(),
                    r.source.empty() ? "(none)" : r.source.c_str(),
                    r.date.empty() ? "?" : r.date.c_str());
        if (r.answer.empty()) findings.push_back(r.id + ": no answer");
        if (not_sources.count(r.source))
            findings.push_back(r.id + ": source is not an official document");
        if (r.date.empty()) findings.push_back(r.id + ": no date read");
    }

    std::printf("\n%zu finding(s)\n", findings.size());
    for (const auto& f : findings) std::printf("  - %s\n", f.c_str());
    std::printf("verdict: %s\n", findings.empty() ? "no mechanical gaps; ready for human review"
                                                  : "NOT ready for review");
    return findings.empty() ? 0 : 1;
}
