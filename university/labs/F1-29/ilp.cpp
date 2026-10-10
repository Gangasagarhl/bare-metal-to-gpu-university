// F1-29 Listing 1: how many cycles do three small instruction sequences take on three
// model machines: 1-wide in-order, 2-wide in-order, and 2-wide out-of-order with a window
// of 8 instructions? Latencies are model values (load 3 cycles, fadd 3, addi 1), not the
// values of any real CPU.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

struct Instr
{
    std::string text;
    int dest, src1, src2;  // register numbers; -1 = none
    int latency;           // cycles until the result can be used
};

// Returns the cycle at which every instruction has finished.
static int simulate(const std::vector<Instr>& prog, int width, bool outOfOrder, int window)
{
    std::vector<int> readyAt(16, 0);          // cycle at which each register's value is ready
    std::vector<int> issued(prog.size(), -1); // cycle at which each instruction issued
    std::size_t next = 0;                     // oldest instruction not yet issued
    int finish = 0;
    for (int cycle = 1; next < prog.size(); ++cycle) {
        int slots = width;
        const std::size_t end = outOfOrder ? std::min(prog.size(), next + window) : prog.size();
        for (std::size_t i = next; i < end && slots > 0; ++i) {
            if (issued[i] >= 0) {
                continue;
            }
            const Instr& in = prog[i];
            const bool ready = (in.src1 < 0 || readyAt[in.src1] <= cycle) &&
                               (in.src2 < 0 || readyAt[in.src2] <= cycle);
            // an out-of-order machine must also not overwrite a register an older,
            // not-yet-issued instruction still has to read or write (renaming would fix this)
            bool clash = false;
            for (std::size_t k = next; k < i && outOfOrder; ++k) {
                if (issued[k] < 0 && (prog[k].src1 == in.dest || prog[k].src2 == in.dest ||
                                      prog[k].dest == in.dest)) {
                    clash = true;
                }
            }
            if (!ready || clash) {
                if (!outOfOrder) {
                    break;  // in-order: nobody may overtake the oldest waiting instruction
                }
                continue;
            }
            issued[i] = cycle;
            if (in.dest >= 0) {
                readyAt[in.dest] = cycle + in.latency;
            }
            finish = std::max(finish, cycle + in.latency - 1);
            --slots;
        }
        while (next < prog.size() && issued[next] >= 0) {
            ++next;
        }
    }
    return finish;
}

int main()
{
    // r1 = pointer, values loaded into r2..r9, running sums in r10..r13
    auto sums = [](int accumulators) {     // load 8 values, fadd each into one of the sums
        std::vector<Instr> p;
        for (int i = 0; i < 8; ++i) {
            const int acc = 10 + i % accumulators;
            p.push_back({"lw", 2 + i, 1, -1, 3});
            p.push_back({"fadd", acc, acc, 2 + i, 3});
        }
        for (int a = 1; a < accumulators; ++a) {   // combine the partial sums at the end
            p.push_back({"fadd", 10, 10, 10 + a, 3});
        }
        return p;
    };
    std::vector<Instr> independent;
    for (int i = 0; i < 16; ++i) {         // sixteen adds that do not depend on each other
        independent.push_back({"addi", 2 + i % 12, 0, -1, 1});
    }
    const std::vector<Instr> chain = sums(1), twoAcc = sums(2), fourAcc = sums(4);
    std::printf("%-34s %6s %18s %18s %18s\n", "sequence", "instr", "1-wide in-order",
                "2-wide in-order", "2-wide out-of-order");
    const std::vector<std::pair<std::string, const std::vector<Instr>*>> all = {
        {"A: 16 independent adds", &independent},
        {"B: 8 x (load, fadd) into ONE sum", &chain},
        {"C: 8 x (load, fadd) into TWO sums", &twoAcc},
        {"D: 8 x (load, fadd) into FOUR sums", &fourAcc}};
    for (const auto& [name, prog] : all) {
        const int n = static_cast<int>(prog->size());
        const int c1 = simulate(*prog, 1, false, 1);
        const int c2 = simulate(*prog, 2, false, 1);
        const int c3 = simulate(*prog, 2, true, 8);
        std::printf("%-34s %6d %8d (IPC %.2f) %8d (IPC %.2f) %8d (IPC %.2f)\n", name.c_str(), n,
                    c1, static_cast<double>(n) / c1, c2, static_cast<double>(n) / c2, c3,
                    static_cast<double>(n) / c3);
    }
    return 0;
}
