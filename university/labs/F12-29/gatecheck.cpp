// gatecheck.cpp - check a design-review gate record before anyone signs it (SE501, F12-29).
// Reads one gate record from standard input:
//   gate <R0..R4> author:<name> reviewers-needed:<n>
//   require <evidence id> [safety]          (entry criterion fixed when the gate was planned)
//   evidence <id> <pass|fail|missing> file:<path>
//   test <name> <pass|fail|skipped>         (the acceptance tests this gate depends on)
//   reviewer <name> <approve|conditions|reject>
//   condition <id> owner:<name|-> due:<day|-> on:<evidence id|->
// Prints every finding and one outcome: PASS, PASS WITH CONDITIONS or NOT YET.
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Condition
{
    std::string id, owner, due, on;
};

static std::string field(const std::string& tok, const std::string& key)
{
    return tok.rfind(key, 0) == 0 ? tok.substr(key.size()) : std::string{};
}

int main()
{
    std::string gate, author;
    int needed = 2;
    std::vector<std::string> required;
    std::set<std::string> safetyItems;
    std::map<std::string, std::string> evidence;  // id -> result
    std::vector<std::pair<std::string, std::string>> tests;
    std::vector<std::pair<std::string, std::string>> reviewers;
    std::vector<Condition> conditions;

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        if (!(in >> kind) || kind[0] == '#') continue;
        if (kind == "gate") {
            std::string a, n;
            in >> gate >> a >> n;
            author = field(a, "author:");
            needed = std::stoi(field(n, "reviewers-needed:"));
        } else if (kind == "require") {
            std::string id, flag;
            in >> id >> flag;
            required.push_back(id);
            if (flag == "safety") safetyItems.insert(id);
        } else if (kind == "evidence") {
            std::string id, result;
            in >> id >> result;
            evidence[id] = result;
        } else if (kind == "test") {
            std::string name, result;
            in >> name >> result;
            tests.emplace_back(name, result);
        } else if (kind == "reviewer") {
            std::string name, verdict;
            in >> name >> verdict;
            reviewers.emplace_back(name, verdict);
        } else if (kind == "condition") {
            Condition c;
            std::string o, d, on;
            in >> c.id >> o >> d >> on;
            c.owner = field(o, "owner:");
            c.due = field(d, "due:");
            c.on = field(on, "on:");
            conditions.push_back(c);
        }
    }

    int blocking = 0;
    std::cout << "gate " << gate << " (author " << author << ")\n";
    for (const auto& id : required) {
        auto it = evidence.find(id);
        std::string result = it == evidence.end() ? "missing" : it->second;
        bool ok = result == "pass";
        std::cout << (ok ? "  ok      " : "  BLOCK   ") << "evidence " << id << ": " << result
                  << (safetyItems.count(id) ? " [safety]" : "") << "\n";
        if (!ok) ++blocking;
    }
    int passed = 0;
    for (const auto& [name, result] : tests) {
        if (result == "pass") {
            ++passed;
            continue;
        }
        std::cout << "  BLOCK   test " << name << ": " << result
                  << (result == "skipped" ? " (a skipped acceptance test is not a pass)" : "")
                  << "\n";
        ++blocking;
    }
    std::cout << "  tests: " << passed << " of " << tests.size() << " pass\n";

    int approvals = 0, withConditions = 0;
    for (const auto& [name, verdict] : reviewers) {
        if (name == author) {
            std::cout << "  BLOCK   reviewer " << name << " is the author (does not count)\n";
            ++blocking;
            continue;
        }
        if (verdict == "reject") {
            std::cout << "  BLOCK   reviewer " << name << " rejects\n";
            ++blocking;
        } else {
            ++approvals;
            if (verdict == "conditions") ++withConditions;
        }
    }
    if (approvals < needed) {
        std::cout << "  BLOCK   independent approvals " << approvals << " of " << needed
                  << " needed\n";
        ++blocking;
    }

    int openConditions = 0;
    for (const auto& c : conditions) {
        if (c.owner.empty() || c.owner == "-" || c.due.empty() || c.due == "-") {
            std::cout << "  BLOCK   condition " << c.id << " has no owner or no due date\n";
            ++blocking;
        } else if (safetyItems.count(c.on)) {
            std::cout << "  BLOCK   condition " << c.id << " defers a safety item (" << c.on
                      << "); safety items must pass before the gate\n";
            ++blocking;
        } else {
            std::cout << "  cond    " << c.id << " owner " << c.owner << " due day " << c.due
                      << "\n";
            ++openConditions;
        }
    }
    if (withConditions > 0 && conditions.empty()) {
        std::cout << "  BLOCK   a reviewer approved with conditions, but none are written down\n";
        ++blocking;
    }

    std::cout << "outcome: ";
    if (blocking > 0) {
        std::cout << "NOT YET (" << blocking << " blocking finding" << (blocking == 1 ? "" : "s")
                  << ")\n";
        return 1;
    }
    if (openConditions > 0) {
        std::cout << "PASS WITH CONDITIONS (" << openConditions << " open)\n";
    } else {
        std::cout << "PASS\n";
    }
    return 0;
}
