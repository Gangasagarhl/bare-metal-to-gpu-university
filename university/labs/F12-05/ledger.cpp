// ledger.cpp - SE301 F12-05: the author's ledger of review comments (stdin).
//   comment <id> <label> <file:line> | <text>
//   reply   <id> fixed <commit>      | <what changed>
//   reply   <id> answered            | <the answer>
//   reply   <id> wontfix             | <the reason>
//   reply   <id> followup <issue>    | <what will be done there>
//   agree   <id> <reviewer>          (the reviewer accepts an answer or a wontfix)
// It checks BOOKKEEPING, not truth: "fixed abc1234" is accepted without looking at abc1234.
#include <cctype>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Item
{
    std::string label;
    std::string where;
    std::string reply;          // fixed, answered, wontfix, followup or empty
    std::string ref;            // commit or issue id
    std::string text;           // text after '|' in the reply
    std::string agreedBy;
};

static std::string afterBar(const std::string& line)
{
    const auto bar = line.find('|');
    if (bar == std::string::npos) return "";
    std::string t = line.substr(bar + 1);
    t.erase(0, t.find_first_not_of(' '));
    return t;
}

static bool looksLikeCommit(const std::string& s)
{
    if (s.size() < 7) return false;
    for (char c : s) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

int main()
{
    std::map<std::string, Item> items;
    std::vector<std::string> order;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line.substr(0, line.find('|')));
        std::string kind, id;
        if (!(in >> kind >> id)) continue;
        if (kind == "comment") {
            Item it;
            in >> it.label >> it.where;
            items[id] = it;
            order.push_back(id);
        } else if (kind == "reply" && items.count(id)) {
            Item& it = items[id];
            in >> it.reply >> it.ref;
            it.text = afterBar(line);
        } else if (kind == "agree" && items.count(id)) {
            in >> items[id].agreedBy;
        }
    }

    int blockers = 0;
    int notes = 0;
    for (const auto& id : order) {
        const Item& it = items[id];
        std::string problem;
        if (it.reply.empty()) {
            problem = "no reply";
        } else if (it.reply == "fixed" && !looksLikeCommit(it.ref)) {
            problem = "\"fixed\" without a commit id";
        } else if (it.reply == "wontfix" && it.text.empty()) {
            problem = "\"wontfix\" without a reason";
        } else if (it.reply == "followup" && it.ref.empty()) {
            problem = "\"followup\" without an issue id";
        } else if (it.label == "blocking" && (it.reply == "wontfix" || it.reply == "answered") &&
                   it.agreedBy.empty()) {
            problem = "blocking comment closed without the reviewer's agreement";
        }
        const bool blocks = !problem.empty() && it.label == "blocking";
        const std::string verdict =
            problem.empty() ? "ok" : problem + (blocks ? "  [BLOCKS MERGE]" : "");
        std::printf("%-3s %-10s %-16s %-9s %-8s %s\n", id.c_str(), it.label.c_str(),
                    it.where.c_str(), it.reply.empty() ? "-" : it.reply.c_str(),
                    it.ref.empty() ? "-" : it.ref.c_str(), verdict.c_str());
        if (blocks) {
            ++blockers;
        } else if (!problem.empty()) {
            ++notes;
        }
    }
    std::printf("%zu comments, %d blocking problems, %d other problems: %s\n", order.size(),
                blockers, notes, blockers == 0 ? "ready to merge" : "NOT ready to merge");
    return blockers == 0 ? 0 : 1;
}
