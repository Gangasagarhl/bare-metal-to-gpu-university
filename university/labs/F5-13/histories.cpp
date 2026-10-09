// histories.cpp - checks small histories of a read/write register against two consistency
// models by brute force (try every order of the operations):
//   linearizable            : some total order is a legal register history AND respects real
//                             time (if A finished before B started, A comes first)
//   sequentially consistent : some total order is legal AND respects each client's own order
// A register starts at 0; a read must return the value of the latest write before it.
// Input (histories.in): "history <name>", then lines "<client> w|r <value> <start> <end>", "end".
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

struct Op
{
    std::string client;
    char kind;      // 'w' or 'r'
    int value;      // value written, or value the read returned
    int start;      // invocation time
    int end;        // response time
};

bool legal(const std::vector<Op>& ops, const std::vector<int>& order)
{
    int reg = 0;
    for (int i : order) {
        if (ops[i].kind == 'w') {
            reg = ops[i].value;
        } else if (ops[i].value != reg) {
            return false;
        }
    }
    return true;
}

// Does `order` put a before b whenever `mustPrecede(a, b)`?
template <typename Rule>
bool respects(const std::vector<Op>& ops, const std::vector<int>& order, Rule mustPrecede)
{
    for (std::size_t x = 0; x < order.size(); ++x) {
        for (std::size_t y = x + 1; y < order.size(); ++y) {
            if (mustPrecede(ops[order[y]], ops[order[x]])) {
                return false;          // y must come before x, but comes after
            }
        }
    }
    return true;
}

template <typename Rule>
std::string search(const std::vector<Op>& ops, Rule mustPrecede)
{
    std::vector<int> order(ops.size());
    std::iota(order.begin(), order.end(), 0);
    do {
        if (respects(ops, order, mustPrecede) && legal(ops, order)) {
            std::string w;
            for (int i : order) {
                w += " " + ops[i].client + ":" + ops[i].kind + std::to_string(ops[i].value);
            }
            return "yes, e.g." + w;
        }
    } while (std::next_permutation(order.begin(), order.end()));
    return "no";
}

int main()
{
    std::string line;
    std::string name;
    std::vector<Op> ops;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string word;
        in >> word;
        if (word == "history") {
            std::getline(in >> std::ws, name);
            ops.clear();
        } else if (word == "end") {
            std::printf("%s\n", name.c_str());
            for (const Op& o : ops) {
                std::printf("    %s %s %d  [%d, %d]\n", o.client.c_str(),
                            o.kind == 'w' ? "write" : "read ->", o.value, o.start, o.end);
            }
            auto realTime = [](const Op& a, const Op& b) { return a.end < b.start; };
            auto program = [](const Op& a, const Op& b) {
                return a.client == b.client && a.end < b.start;
            };
            std::printf("  linearizable:            %s\n", search(ops, realTime).c_str());
            std::printf("  sequentially consistent: %s\n\n", search(ops, program).c_str());
        } else if (!word.empty() && word[0] != '#') {
            Op o;
            o.client = word;
            in >> o.kind >> o.value >> o.start >> o.end;
            ops.push_back(o);
        }
    }
    return 0;
}
