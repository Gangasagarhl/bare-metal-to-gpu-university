// mp8plan.cpp - check and schedule a mega-project plan (SE501, chapter F12-28).
// Reads a plan from standard input:
//   milestone <id> <optimistic> <likely> <pessimistic> test:<name> deps:<a,b|-> [why:<text>]
//   gate <name> after:<milestone id>
//   done <id> <actual days>        (progress so far)
//   blocked <id> by:<id> <days>    (time a milestone waited on something)
// Durations are working days. Prints: checks, schedule, critical path, gates, tracking.
#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Milestone
{
    std::string id;
    double opt = 0, likely = 0, pess = 0;
    std::string test;
    std::vector<std::string> deps;
    double startL = 0, finishL = 0, startP = 0, finishP = 0, slack = 0;
    double actual = -1;  // -1 = not done
};

static std::vector<std::string> splitComma(const std::string& s)
{
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, ',')) {
        if (!part.empty() && part != "-") out.push_back(part);
    }
    return out;
}

int main()
{
    std::vector<Milestone> ms;
    std::map<std::string, std::size_t> index;
    std::vector<std::pair<std::string, std::string>> gates;      // gate name, milestone
    std::vector<std::pair<std::string, std::string>> blockedBy;  // waiter, blocker
    std::map<std::string, double> blockedDays;
    int problems = 0;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        if (!(in >> kind) || kind[0] == '#') continue;
        if (kind == "milestone") {
            Milestone m;
            in >> m.id >> m.opt >> m.likely >> m.pess;
            std::string tok;
            while (in >> tok) {
                if (tok.rfind("test:", 0) == 0) m.test = tok.substr(5);
                else if (tok.rfind("deps:", 0) == 0) m.deps = splitComma(tok.substr(5));
            }
            index[m.id] = ms.size();
            ms.push_back(m);
        } else if (kind == "gate") {
            std::string name, after;
            in >> name >> after;
            gates.emplace_back(name, after.substr(6));
        } else if (kind == "done") {
            std::string id;
            double days = 0;
            in >> id >> days;
            if (index.count(id)) ms[index[id]].actual = days;
        } else if (kind == "blocked") {
            std::string id, by;
            double days = 0;
            in >> id >> by >> days;
            blockedBy.emplace_back(id, by.substr(3));
            blockedDays[id + " by " + by.substr(3)] += days;
        }
    }

    std::cout << "== checks\n";
    for (const auto& m : ms) {
        if (m.test.empty() || m.test == "tbd") {
            std::cout << "PROBLEM " << m.id << ": no acceptance test\n";
            ++problems;
        }
        if (!(m.opt <= m.likely && m.likely <= m.pess)) {
            std::cout << "PROBLEM " << m.id << ": estimates not ordered o <= m <= p\n";
            ++problems;
        }
        for (const auto& d : m.deps) {
            if (!index.count(d)) {
                std::cout << "PROBLEM " << m.id << ": unknown dependency " << d << "\n";
                ++problems;
            }
        }
    }
    for (const auto& [waiter, blocker] : blockedBy) {
        if (!index.count(waiter)) continue;
        double& days = blockedDays[waiter + " by " + blocker];
        if (days == 0) continue;  // already reported
        if (!index.count(blocker)) {
            std::cout << "PROBLEM " << waiter << " waited " << days << " days on " << blocker
                      << ", which is not a milestone in the plan (hidden work)\n";
            ++problems;
            days = 0;
            continue;
        }
        const auto& deps = ms[index[waiter]].deps;
        if (std::find(deps.begin(), deps.end(), blocker) == deps.end()) {
            std::cout << "PROBLEM " << waiter << " waited " << days << " days on " << blocker
                      << ", which is not in its deps (undeclared dependency)\n";
            ++problems;
        }
        days = 0;  // report each pair once
    }

    // Topological order (Kahn); anything left over is in a cycle.
    std::vector<int> indegree(ms.size(), 0);
    for (const auto& m : ms) {
        for (const auto& d : m.deps) {
            if (index.count(d)) ++indegree[index[m.id]];
        }
    }
    std::vector<std::size_t> order;
    for (std::size_t i = 0; i < ms.size(); ++i) {
        if (indegree[i] == 0) order.push_back(i);
    }
    for (std::size_t k = 0; k < order.size(); ++k) {
        for (std::size_t j = 0; j < ms.size(); ++j) {
            const auto& d = ms[j].deps;
            if (std::find(d.begin(), d.end(), ms[order[k]].id) != d.end() && --indegree[j] == 0) {
                order.push_back(j);
            }
        }
    }
    if (order.size() != ms.size()) {
        std::cout << "PROBLEM dependency cycle among:";
        for (std::size_t i = 0; i < ms.size(); ++i) {
            if (indegree[i] > 0) std::cout << " " << ms[i].id;
        }
        std::cout << "\n";
        return 1;
    }
    if (problems == 0) std::cout << "no problems found\n";

    // Forward pass with likely and with pessimistic durations.
    for (auto i : order) {
        auto& m = ms[i];
        for (const auto& d : m.deps) {
            if (!index.count(d)) continue;
            m.startL = std::max(m.startL, ms[index[d]].finishL);
            m.startP = std::max(m.startP, ms[index[d]].finishP);
        }
        m.finishL = m.startL + m.likely;
        m.finishP = m.startP + m.pess;
    }
    double endL = 0, endP = 0, endO = 0;
    for (const auto& m : ms) {
        endL = std::max(endL, m.finishL);
        endP = std::max(endP, m.finishP);
    }
    // Optimistic end, for the range.
    std::map<std::string, double> finO;
    for (auto i : order) {
        double s = 0;
        for (const auto& d : ms[i].deps) {
            if (finO.count(d)) s = std::max(s, finO[d]);
        }
        finO[ms[i].id] = s + ms[i].opt;
        endO = std::max(endO, finO[ms[i].id]);
    }
    // Backward pass (likely): latest finish, slack.
    std::map<std::string, double> latestFinish;
    for (const auto& m : ms) latestFinish[m.id] = endL;
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        const auto& m = ms[*it];
        double latestStart = latestFinish[m.id] - m.likely;
        for (const auto& d : m.deps) {
            if (index.count(d)) latestFinish[d] = std::min(latestFinish[d], latestStart);
        }
    }
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "\n== schedule (working days; L = likely, P = pessimistic)\n";
    std::cout << "id    start L  finish L  finish P  slack  on critical path\n";
    for (auto i : order) {
        auto& m = ms[i];
        m.slack = latestFinish[m.id] - m.finishL;
        std::cout << std::left << std::setw(6) << m.id << std::right << std::setw(7) << m.startL
                  << std::setw(10) << m.finishL << std::setw(10) << m.finishP << std::setw(7)
                  << m.slack << "  " << (m.slack < 0.05 ? "yes" : "") << "\n";
    }
    std::cout << "end: optimistic " << endO << ", likely " << endL << ", pessimistic " << endP
              << " days\n";
    std::cout << "critical path (likely):";
    for (auto i : order) {
        if (ms[i].slack < 0.05) std::cout << " " << ms[i].id;
    }
    std::cout << "\n";

    std::cout << "\n== gates\n";
    for (const auto& [name, after] : gates) {
        if (!index.count(after)) {
            std::cout << name << ": after unknown milestone " << after << "\n";
            continue;
        }
        const auto& m = ms[index[after]];
        std::cout << name << " after " << after << ": day " << m.finishL << " likely, "
                  << m.finishP << " pessimistic\n";
    }

    double sumActual = 0, sumLikely = 0, remaining = 0;
    int doneCount = 0;
    for (const auto& m : ms) {
        if (m.actual >= 0) {
            sumActual += m.actual;
            sumLikely += m.likely;
            ++doneCount;
        }
    }
    if (doneCount > 0) {
        double ratio = sumActual / sumLikely;
        std::cout << "\n== tracking\n";
        for (const auto& m : ms) {
            if (m.actual >= 0) {
                std::cout << m.id << ": likely " << m.likely << ", actual " << m.actual
                          << " (x" << std::setprecision(2) << m.actual / m.likely
                          << std::setprecision(1) << ")\n";
            }
        }
        // Re-forecast: remaining milestones on the critical path scaled by the observed ratio.
        for (auto i : order) {
            if (ms[i].actual < 0 && ms[i].slack < 0.05) remaining += ms[i].likely;
        }
        std::cout << "done " << doneCount << " of " << ms.size() << "; actual/likely so far x"
                  << std::setprecision(2) << ratio << std::setprecision(1) << "\n";
        std::cout << "remaining critical-path work: " << remaining << " likely days, "
                  << remaining * ratio << " at the observed ratio\n";
        // Forecast: done milestones at their actual length, the rest at likely x ratio.
        std::map<std::string, double> fin;
        double endF = 0;
        for (auto i : order) {
            double s = 0;
            for (const auto& d : ms[i].deps) {
                if (fin.count(d)) s = std::max(s, fin[d]);
            }
            fin[ms[i].id] = s + (ms[i].actual >= 0 ? ms[i].actual : ms[i].likely * ratio);
            endF = std::max(endF, fin[ms[i].id]);
        }
        std::cout << "forecast end: day " << endF << " (plan said " << endL << " likely, "
                  << endP << " pessimistic)\n";
    }
    return problems == 0 ? 0 : 3;
}
