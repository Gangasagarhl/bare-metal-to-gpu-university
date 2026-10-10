// F8-06 forensic evidence: "the 4-GPU curves do not match". The same training as Listing 1
// on 4 workers, run three ways: correct, and with each of two planted bugs (train.hpp, Mode).
// The evidence printed is what a team would log: the loss every 50 steps and the largest
// difference between the workers' parameter copies.
#include <cstdio>
#include <string>
#include "train.hpp"

int main()
{
    const int steps = 300;
    const Run one = train(1, steps, Mode::Correct);
    const Run runs[3] = {train(4, steps, Mode::Correct), train(4, steps, Mode::NoBroadcast),
                         train(4, steps, Mode::SumNotAverage)};
    const char* names[3] = {"run A (4 workers)", "run B (4 workers)", "run C (4 workers)"};
    std::printf("%-24s", "step");
    for (int s = 0; s < steps; s += 50) { std::printf("%9d", s); }
    std::printf("%9d\n%-24s", steps - 1, "1 worker: loss");
    for (int s = 0; s < steps; s += 50) { std::printf("%9.4f", static_cast<double>(one.loss[static_cast<std::size_t>(s)])); }
    std::printf("%9.4f\n", static_cast<double>(one.loss.back()));
    for (int k = 0; k < 3; ++k) {
        std::printf("%-24s", (std::string(names[k]) + ": loss").c_str());
        for (int s = 0; s < steps; s += 50) { std::printf("%9.4f", static_cast<double>(runs[k].loss[static_cast<std::size_t>(s)])); }
        std::printf("%9.4f\n%-24s", static_cast<double>(runs[k].loss.back()), "   replica gap");
        for (int s = 0; s < steps; s += 50) { std::printf("%9.4f", static_cast<double>(runs[k].replicaGap[static_cast<std::size_t>(s)])); }
        std::printf("%9.4f\n", static_cast<double>(runs[k].replicaGap.back()));
    }
    std::printf("(replica gap = largest |parameter of worker w - same parameter of worker 0| after the step)\n");
    return 0;
}
