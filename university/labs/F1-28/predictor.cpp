// F1-28 Listing 1: four branch predictors on the branch of a nested loop, and what the
// mispredictions cost in a five-stage pipeline that finds branches in EX (2 lost cycles).
#include <cstdio>
#include <string>
#include <vector>
#include "../F1-26/pipe.h"

struct Predictor
{
    std::string name;
    int state;   // meaning depends on kind
    int kind;    // 0 always not taken, 1 always taken, 2 one-bit, 3 two-bit counter

    bool predict() const
    {
        switch (kind) {
        case 0: return false;
        case 1: return true;
        case 2: return state == 1;          // 1 = "taken last time"
        default: return state >= 2;         // 0,1 predict not taken; 2,3 predict taken
        }
    }

    void update(bool taken)
    {
        if (kind == 2) {
            state = taken ? 1 : 0;
        } else if (kind == 3) {
            state = taken ? (state < 3 ? state + 1 : 3) : (state > 0 ? state - 1 : 0);
        }
    }
};

int main()
{
    // The inner loop's closing branch: taken 3 times, then not taken (loop exit), 10 times.
    std::vector<bool> outcomes;
    for (int outer = 0; outer < 10; ++outer) {
        for (int inner = 0; inner < 4; ++inner) {
            outcomes.push_back(inner < 3);
        }
    }
    std::printf("branch outcomes (T taken, N not taken), first 12: ");
    for (std::size_t i = 0; i < 12; ++i) {
        std::printf("%c", outcomes[i] ? 'T' : 'N');
    }
    std::printf(" ... (%zu in total)\n", outcomes.size());
    std::printf("%-22s %8s %14s %9s %18s\n", "predictor", "branches", "mispredictions",
                "accuracy", "cycles lost (x2)");
    std::vector<Predictor> ps = {{"always not taken", 0, 0}, {"always taken", 0, 1},
                                 {"1-bit (last outcome)", 0, 2}, {"2-bit counter", 0, 3}};
    for (Predictor& p : ps) {
        int wrong = 0;
        std::string trace;
        for (const bool t : outcomes) {
            const bool guess = p.predict();
            if (guess != t) {
                ++wrong;
            }
            if (trace.size() < 12) {
                trace += guess == t ? '.' : 'X';
            }
            p.update(t);
        }
        const double acc = 100.0 * (outcomes.size() - wrong) / outcomes.size();
        std::printf("%-22s %8zu %14d %8.1f%% %18d   first 12: %s\n", p.name.c_str(),
                    outcomes.size(), wrong, acc, 2 * wrong, trace.c_str());
    }
    // The same idea inside the pipeline model: the sum loop of F1-23 with forwarding.
    std::istringstream sum(
        ".data 7 3 9 4 6\n addi r1, r0, 0\n addi r2, r0, 5\n addi r3, r0, 0\n"
        "loop: lw r4, 0(r1)\n addi r1, r1, 1\n addi r2, r2, -1\n add r3, r3, r4\n"
        " bne r2, r0, loop\n out r3\n halt\n");
    const u16::Program prog = u16::assemble(sum);
    const pipe::Result guessNotTaken = pipe::schedule(prog, pipe::Config{true, true, true});
    const pipe::Result perfect = pipe::schedule(prog, pipe::Config{true, true, false});
    std::printf("reordered sum loop, forwarding, predict not taken: ");
    pipe::summary(guessNotTaken);
    std::printf("reordered sum loop, forwarding, perfect prediction: ");
    pipe::summary(perfect);
    return 0;
}
