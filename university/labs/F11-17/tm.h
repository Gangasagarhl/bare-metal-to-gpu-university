// tm.h: the university's tiny threat-modelling engine (SS401, used in F11-14 to F11-17).
// It reads a data-flow diagram written as text, checks a few drawing rules, finds the
// flows that cross a trust boundary and lists STRIDE threats per element. The table of
// which STRIDE letters apply to which element type is the course's reading of Shostack's
// "STRIDE per element" idea; it is marked unverified in F11-14 (check the book's table).
#pragma once
#include <cstdio>
#include <istream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace tmod {

enum class Kind { External, Process, Store, Flow };

struct Item
{
    Kind kind = Kind::Process;
    std::string name;            // for a flow: "from -> to : data"
    std::string zone;            // for a flow: "zoneA|zoneB" when it crosses a boundary
    std::string from, to;        // flows only
    std::set<std::string> controls;
    bool crossing = false;       // flow crosses a boundary, or element sends/receives one that does
};

struct Model
{
    std::string title;
    std::vector<Item> items;
    std::vector<std::string> problems;
};

inline std::string trim(const std::string& s)
{
    const std::size_t a = s.find_first_not_of(' ');
    if (a == std::string::npos) {
        return "";
    }
    const std::size_t b = s.find_last_not_of(' ');
    return s.substr(a, b - a + 1);
}

// splits "text [c1,c2]" into text and the set {c1, c2}
inline std::string takeControls(const std::string& s, std::set<std::string>& out)
{
    const std::size_t open = s.rfind('[');
    if (open == std::string::npos || s.back() != ']') {
        return trim(s);
    }
    std::string list = s.substr(open + 1, s.size() - open - 2);
    std::size_t start = 0;
    while (start <= list.size()) {
        std::size_t comma = list.find(',', start);
        if (comma == std::string::npos) {
            comma = list.size();
        }
        const std::string c = trim(list.substr(start, comma - start));
        if (!c.empty()) {
            out.insert(c);
        }
        start = comma + 1;
    }
    return trim(s.substr(0, open));
}

inline const Item* find(const Model& m, const std::string& name)
{
    for (const Item& it : m.items) {
        if (it.kind != Kind::Flow && it.name == name) {
            return &it;
        }
    }
    return nullptr;
}

// Format, one statement per line ('#' starts a comment):
//   title <text>
//   external|process|store <zone> <name> [controls]
//   flow <from name> -> <to name> : <data> [controls]
inline Model parse(std::istream& in)
{
    Model m;
    std::string line;
    int n = 0;
    while (std::getline(in, line)) {
        ++n;
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const std::size_t sp = line.find(' ');
        const std::string word = line.substr(0, sp);
        const std::string rest = sp == std::string::npos ? "" : trim(line.substr(sp + 1));
        if (word == "title") {
            m.title = rest;
        } else if (word == "external" || word == "process" || word == "store") {
            Item it;
            it.kind = word == "external" ? Kind::External
                    : word == "process"  ? Kind::Process : Kind::Store;
            const std::size_t sp2 = rest.find(' ');
            it.zone = rest.substr(0, sp2);
            it.name = takeControls(rest.substr(sp2 + 1), it.controls);
            m.items.push_back(it);
        } else if (word == "flow") {
            Item it;
            it.kind = Kind::Flow;
            const std::size_t arrow = rest.find(" -> ");
            const std::size_t colon = rest.find(" : ");
            if (arrow == std::string::npos || colon == std::string::npos || colon < arrow) {
                m.problems.push_back("line " + std::to_string(n) + ": flow needs 'A -> B : data'");
                continue;
            }
            it.from = trim(rest.substr(0, arrow));
            it.to = trim(rest.substr(arrow + 4, colon - arrow - 4));
            const std::string data = takeControls(rest.substr(colon + 3), it.controls);
            it.name = it.from + " -> " + it.to + " : " + data;
            m.items.push_back(it);
        } else {
            m.problems.push_back("line " + std::to_string(n) + ": unknown word '" + word + "'");
        }
    }
    // drawing rules and boundary crossings
    std::set<std::string> zones;
    for (const Item& it : m.items) {
        if (it.kind != Kind::Flow) {
            zones.insert(it.zone);
        }
    }
    for (Item& f : m.items) {
        if (f.kind != Kind::Flow) {
            continue;
        }
        const Item* a = find(m, f.from);
        const Item* b = find(m, f.to);
        if (a == nullptr || b == nullptr) {
            m.problems.push_back("flow names an unknown element: " + f.name);
            continue;
        }
        if (a->kind != Kind::Process && b->kind != Kind::Process) {
            m.problems.push_back("flow with no process at either end: " + f.name);
        }
        if (a->zone != b->zone) {
            f.crossing = true;
            f.zone = a->zone + "|" + b->zone;
        } else {
            f.zone = a->zone;
        }
    }
    for (Item& e : m.items) {
        if (e.kind == Kind::Flow) {
            continue;
        }
        bool used = false;
        for (const Item& f : m.items) {
            if (f.kind == Kind::Flow && (f.from == e.name || f.to == e.name)) {
                used = true;
                e.crossing = e.crossing || f.crossing;
            }
        }
        if (!used) {
            m.problems.push_back("element with no flows: " + e.name);
        }
    }
    if (zones.size() < 2) {
        m.problems.push_back("only one zone: the diagram has no trust boundary at all");
    }
    return m;
}

// STRIDE letters that apply per element type (course's reading of STRIDE per element)
inline std::string lettersFor(const Item& it)
{
    switch (it.kind) {
    case Kind::External: return "SR";
    case Kind::Process:  return "STRIDE";
    case Kind::Store:    return it.controls.count("log") != 0 ? "TRID" : "TID";
    case Kind::Flow:     return "TID";
    }
    return "";
}

// which claimed control answers which letter
inline const char* controlFor(char letter)
{
    switch (letter) {
    case 'S': return "auth";
    case 'T': return "mac";
    case 'R': return "audit";
    case 'I': return "enc";
    case 'D': return "ratelimit";
    case 'E': return "leastpriv";
    }
    return "";
}

inline const char* kindName(Kind k)
{
    switch (k) {
    case Kind::External: return "external";
    case Kind::Process:  return "process";
    case Kind::Store:    return "store";
    case Kind::Flow:     return "flow";
    }
    return "";
}

inline int report(const Model& m)
{
    std::printf("model: %s\n", m.title.c_str());
    std::map<Kind, int> count;
    std::set<std::string> zones;
    for (const Item& it : m.items) {
        ++count[it.kind];
        if (it.kind != Kind::Flow) {
            zones.insert(it.zone);
        }
    }
    std::printf("elements: %d external, %d process, %d store; flows: %d; zones:",
                count[Kind::External], count[Kind::Process], count[Kind::Store],
                count[Kind::Flow]);
    for (const std::string& z : zones) {
        std::printf(" %s", z.c_str());
    }
    std::printf("\n");
    for (const std::string& p : m.problems) {
        std::printf("PROBLEM: %s\n", p.c_str());
    }
    std::printf("flows crossing a trust boundary:\n");
    int crossings = 0;
    for (const Item& it : m.items) {
        if (it.kind == Kind::Flow && it.crossing) {
            std::printf("  %-44s [%s]\n", it.name.c_str(), it.zone.c_str());
            ++crossings;
        }
    }
    if (crossings == 0) {
        std::printf("  (none)\n");
    }
    // threats: boundary-touching items first, then the rest
    int id = 0, open = 0, claimed = 0;
    std::printf("threats (STRIDE per element), boundary first:\n");
    for (int pass = 0; pass < 2; ++pass) {
        for (const Item& it : m.items) {
            if (it.crossing != (pass == 0)) {
                continue;
            }
            for (char c : lettersFor(it)) {
                ++id;
                const char* ctl = controlFor(c);
                const bool has = it.controls.count(ctl) != 0;
                has ? ++claimed : ++open;
                std::printf("  T%02d %c %-8s %-44s %s%s\n", id, c, kindName(it.kind),
                            it.name.c_str(), has ? "claimed: " : "open",
                            has ? ctl : "");
            }
        }
    }
    std::printf("total: %d threats, %d open, %d with a claimed control (verify each one)\n",
                id, open, claimed);
    return m.problems.empty() ? 0 : 1;
}

} // namespace tmod
