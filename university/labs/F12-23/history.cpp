// history.cpp - "the project that was always 90 % done": plan versus actual.
// Input lines (hours):
//   task <id> <estimate>                  original plan
//   week <n> <reported %> <hours logged>  the weekly status report
//   add  <week> <id> <estimate>           work discovered after the plan was made
//   done <week> <id> <actual hours>       a task closed (its acceptance test passed)
// Without arguments the program prints the evidence a tracker would export.
// With the argument "analyse" it also recomputes progress the honest way.
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Item
{
    double estimate = 0;
    double actual = 0;
    int addedWeek = 0;  // 0 = in the original plan
    int doneWeek = 0;   // 0 = still open
};

struct Week
{
    int n = 0;
    double reported = 0;
    double hours = 0;
};

int main(int argc, char** argv)
{
    const bool analyse = argc > 1 && std::string(argv[1]) == "analyse";
    std::map<std::string, Item> items;
    std::vector<std::string> order;
    std::vector<Week> weeks;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind, id;
        if (!(in >> kind) || kind[0] == '#') {
            continue;
        }
        Item it;
        int wk = 0;
        double v = 0;
        if (kind == "task" && in >> id >> it.estimate) {
            items[id] = it;
            order.push_back(id);
        } else if (kind == "add" && in >> wk >> id >> it.estimate) {
            it.addedWeek = wk;
            items[id] = it;
            order.push_back(id);
        } else if (kind == "done" && in >> wk >> id >> v && items.count(id) == 1) {
            items[id].doneWeek = wk;
            items[id].actual = v;
        } else if (kind == "week") {
            Week w;
            if (!(in >> w.n >> w.reported >> w.hours)) {
                std::cout << "cannot read: " << line << '\n';
                return 2;
            }
            weeks.push_back(w);
        } else {
            std::cout << "cannot read: " << line << '\n';
            return 2;
        }
    }
    double planTotal = 0;
    for (const auto& [id, it] : items) {
        if (it.addedWeek == 0) {
            planTotal += it.estimate;
        }
    }
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "original plan: " << planTotal << " h of estimated work\n\n";
    std::cout << "week  reported  hours(cum)  tasks done/known";
    if (analyse) {
        std::cout << "  hours/plan  earned  scope(h)";
    }
    std::cout << '\n';
    double cum = 0;
    for (const Week& w : weeks) {
        cum += w.hours;
        int done = 0, known = 0;
        double scope = 0, earned = 0;
        for (const auto& [id, it] : items) {
            if (it.addedWeek <= w.n) {
                ++known;
                scope += it.estimate;
                if (it.doneWeek != 0 && it.doneWeek <= w.n) {
                    ++done;
                    earned += it.estimate;
                }
            }
        }
        std::cout << std::setw(4) << w.n << std::setw(9) << w.reported << " %"
                  << std::setw(11) << cum
                  << std::setw(10) << done << '/' << known;
        if (analyse) {
            std::cout << std::setw(16) << 100.0 * cum / planTotal << " %" << std::setw(6)
                      << 100.0 * earned / scope << " %" << std::setw(9) << scope;
        }
        std::cout << '\n';
    }
    std::cout << "\ntask         planned?      estimate  actual  actual/estimate\n";
    double estAll = 0, actAll = 0;
    for (const std::string& id : order) {
        const Item& it = items[id];
        estAll += it.estimate;
        actAll += it.actual;
        const std::string origin =
            it.addedWeek == 0 ? "plan" : "added wk " + std::to_string(it.addedWeek);
        std::cout << std::left << std::setw(13) << id << std::setw(12) << origin << std::right
                  << std::setw(10) << it.estimate << std::setw(8) << it.actual;
        if (it.doneWeek != 0) {
            std::cout << std::setw(12) << it.actual / it.estimate;
        } else {
            std::cout << "        open";
        }
        std::cout << '\n';
    }
    std::cout << "all tasks: estimate " << estAll << " h, actual " << actAll << " h, hours logged "
              << cum << " h\n";
    if (analyse) {
        double addedEst = 0, addedAct = 0, planAct = 0;
        for (const auto& [id, it] : items) {
            if (it.addedWeek != 0) {
                addedEst += it.estimate;
                addedAct += it.actual;
            } else {
                planAct += it.actual;
            }
        }
        std::cout << "\nanalysis:\n";
        std::cout << "  planned tasks: estimate " << planTotal << " h, actual " << planAct
                  << " h (x"
                  << planAct / planTotal << ")\n";
        std::cout << "  discovered tasks: estimate " << addedEst << " h, actual " << addedAct
                  << " h, " << 100.0 * addedAct / actAll << " % of all hours\n";
        std::cout << "  whole project took " << cum / planTotal << " x the original plan\n";
        int ninety = 0;
        for (const Week& w : weeks) {
            if (w.reported >= 90.0 && w.reported < 100.0) {
                ++ninety;
            }
        }
        std::cout << "  weeks reported at 90-99 %: " << ninety << '\n';
    }
    return 0;
}
