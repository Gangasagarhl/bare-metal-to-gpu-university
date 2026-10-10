// F11-13 Listing 1: an attack tree, evaluated. An OR node is as cheap as its cheapest
// child; an AND node costs the sum of its children. Costs are "effort points" chosen for
// the exercise (relative guesses written by the modeller, not measurements). MITIGATE
// lines raise the cost of one leaf; the program applies them one by one and shows how the
// cheapest path moves.
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Node
{
    std::string kind;   // "OR", "AND" or "LEAF"
    std::string text;
    int cost = 0;       // leaves only
    int depth = 0;
    std::vector<int> kids;
};

struct Mitigation
{
    std::string leaf;
    int extra = 0;
    std::string how;
};

// returns the cheapest cost of node i and appends the leaves of that cheapest way to path
int cheapest(const std::vector<Node>& t, int i, std::vector<int>& path)
{
    const Node& n = t[static_cast<std::size_t>(i)];
    if (n.kind == "LEAF") {
        path.push_back(i);
        return n.cost;
    }
    if (n.kind == "AND") {
        int sum = 0;
        for (int k : n.kids) {
            sum += cheapest(t, k, path);
        }
        return sum;
    }
    int best = -1;
    std::vector<int> bestPath;
    for (int k : n.kids) {
        std::vector<int> p;
        const int c = cheapest(t, k, p);
        if (best < 0 || c < best) {
            best = c;
            bestPath = p;
        }
    }
    path.insert(path.end(), bestPath.begin(), bestPath.end());
    return best;
}

void report(const std::vector<Node>& t, const char* when)
{
    std::vector<int> path;
    const int c = cheapest(t, 0, path);
    std::printf("%s: cheapest attack costs %d points\n", when, c);
    for (int i : path) {
        const Node& n = t[static_cast<std::size_t>(i)];
        std::printf("    %3d  %s\n", n.cost, n.text.c_str());
    }
    for (int k : t[0].kids) {
        std::vector<int> p;
        std::printf("    branch %3d  %s\n", cheapest(t, k, p),
                    t[static_cast<std::size_t>(k)].text.c_str());
    }
}

int main()
{
    std::vector<Node> tree;
    std::vector<Mitigation> mits;
    std::vector<int> stack;   // index of the open node at each depth
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (line.rfind("MITIGATE ", 0) == 0) {
            // MITIGATE <extra> <leaf text> | <how>
            Mitigation m;
            std::size_t sp = line.find(' ', 9);
            m.extra = std::stoi(line.substr(9, sp - 9));
            const std::size_t bar = line.find(" | ");
            m.leaf = line.substr(sp + 1, bar - sp - 1);
            m.how = line.substr(bar + 3);
            mits.push_back(m);
            continue;
        }
        Node n;
        std::size_t ind = line.find_first_not_of(' ');
        n.depth = static_cast<int>(ind / 2);
        std::size_t sp = line.find(' ', ind);
        n.kind = line.substr(ind, sp - ind);
        std::string rest = line.substr(sp + 1);
        if (n.kind == "LEAF") {
            std::size_t sp2 = rest.find(' ');
            n.cost = std::stoi(rest.substr(0, sp2));
            rest = rest.substr(sp2 + 1);
        }
        n.text = rest;
        const int idx = static_cast<int>(tree.size());
        stack.resize(static_cast<std::size_t>(n.depth));
        if (n.depth > 0) {
            tree[static_cast<std::size_t>(stack.back())].kids.push_back(idx);
        }
        tree.push_back(n);
        stack.push_back(idx);
    }
    if (tree.empty()) {
        std::printf("no tree\n");
        return 1;
    }
    std::printf("goal: %s (%zu nodes)\n", tree[0].text.c_str(), tree.size());
    report(tree, "before");
    for (const Mitigation& m : mits) {
        bool found = false;
        for (Node& n : tree) {
            if (n.kind == "LEAF" && n.text == m.leaf) {
                n.cost += m.extra;
                found = true;
            }
        }
        if (!found) {
            std::printf("MITIGATE names no leaf: %s\n", m.leaf.c_str());
            return 1;
        }
        std::printf("\nmitigation: %s (+%d on \"%s\")\n", m.how.c_str(), m.extra, m.leaf.c_str());
        report(tree, "after");
    }
    return 0;
}
