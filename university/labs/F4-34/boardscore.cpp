// boardscore.cpp - F4-34: choose a first board by its documentation, not by its speed.
//
// Reads board records (standard input), applies three hard gates (a debug console, a recovery
// path, a boot chain that runs your code), scores what is left, ranks the boards and prints the
// first steps of a bring-up plan for the winner. The weights are this course's choice for a
// FIRST bring-up board; argue with them in the mini-project.
#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Board {
    std::string name;
    std::map<std::string, std::string> f;   // field -> value, as written in the record
    int score = 0;
    std::vector<std::string> gate_failures;
};

namespace {

int points(const Board& b, const std::string& field, const std::map<std::string, int>& table)
{
    auto it = b.f.find(field);
    if (it == b.f.end()) {
        return 0;                           // unknown counts as nothing: go and find out
    }
    auto p = table.find(it->second);
    return p == table.end() ? 0 : p->second;
}

std::string get(const Board& b, const std::string& field)
{
    auto it = b.f.find(field);
    return it == b.f.end() ? "unknown" : it->second;
}

void evaluate(Board& b)
{
    // Gates: without these a beginner cannot make progress at all.
    if (get(b, "uart") != "yes") {
        b.gate_failures.push_back("no debug UART");
    }
    if (get(b, "recovery") != "usb" && get(b, "recovery") != "sd") {
        b.gate_failures.push_back("no recovery path");
    }
    if (get(b, "boot_chain") != "open" && get(b, "boot_chain") != "payload") {   // only known-good values pass
        b.gate_failures.push_back("boot chain does not run your code");
    }
    b.score = points(b, "trm", {{"full", 30}, {"partial", 15}}) +
              points(b, "errata", {{"yes", 5}}) +
              points(b, "boot_chain", {{"open", 20}, {"payload", 10}}) +
              points(b, "dram_init", {{"open", 5}}) +
              points(b, "mainline_linux", {{"yes", 15}, {"partial", 8}}) +
              points(b, "mainline_uboot", {{"yes", 10}}) +
              points(b, "recovery", {{"usb", 10}, {"sd", 8}}) +
              points(b, "uart", {{"yes", 5}});
}

}  // namespace

int main()
{
    std::vector<Board> boards;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string key;
        std::string value;
        if (!(in >> key) || key[0] == '#') {
            continue;
        }
        std::getline(in >> std::ws, value);
        if (key == "board") {
            boards.push_back(Board{value, {}, 0, {}});
        } else if (!boards.empty()) {
            boards.back().f[key] = value;
        }
    }
    for (Board& b : boards) {
        evaluate(b);
    }
    std::stable_sort(boards.begin(), boards.end(), [](const Board& a, const Board& b) {
        if (a.gate_failures.empty() != b.gate_failures.empty()) {
            return a.gate_failures.empty();  // boards that pass every gate come first
        }
        return a.score > b.score;
    });
    std::cout << "rank  score  board                      gates\n";
    int rank = 1;
    for (const Board& b : boards) {
        std::string gates = b.gate_failures.empty() ? "pass" : "FAIL:";
        for (const std::string& g : b.gate_failures) {
            gates += " " + g + ";";
        }
        std::cout << "  " << rank++ << "    " << (b.score < 10 ? " " : "") << b.score << "    " << b.name
                  << std::string(b.name.size() < 27 ? 27 - b.name.size() : 1, ' ') << gates << "\n";
    }
    if (boards.empty() || !boards.front().gate_failures.empty()) {
        std::cout << "no board passes the gates: collect better records first\n";
        return 1;
    }
    const Board& w = boards.front();
    std::cout << "\nbring-up plan, first steps, for " << w.name << ":\n"
              << "  1. connect the debug UART (" << get(w, "uart_note") << "); see the first boot messages\n"
              << "  2. practise recovery (" << get(w, "recovery") << ") before writing to on-board storage\n"
              << "  3. boot the known-good image; save its devicetree and kernel log as the reference\n"
              << "  4. read the devicetree: memory, CPUs, interrupt controller, timer, UART, clocks, resets, pins\n"
              << "  5. bring up in order: MMU, interrupts, timer, UART, GPIO heartbeat, storage, then the rest\n"
              << "  documents to collect: TRM (" << get(w, "trm") << "), errata (" << get(w, "errata")
              << "), board schematic, the vendor's devicetree source\n";
    return 0;
}
