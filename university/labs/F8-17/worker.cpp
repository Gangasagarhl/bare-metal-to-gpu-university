// F8-17 Listing 4: a training process that a scheduler can stop politely. It stands in for the
// real trainer: each step updates one number with a fixed rule and sleeps for step_ms.
//  - with ckpt=<file> it resumes from that file and writes it every `every` steps
//    (write to <file>.tmp, then rename: a reader sees the old or the new file, never half of one);
//  - on SIGTERM it finishes the current step, writes a checkpoint and exits with code 99,
//    which the job script reads as "stopped early on purpose: requeue me".
// Arguments: steps=<n> every=<k> step_ms=<ms> ckpt=<file> (defaults: 5 steps, no checkpoint file)
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

volatile std::sig_atomic_t stopRequested = 0;

extern "C" void onTerm(int)
{
    stopRequested = 1;                 // only set a flag: the main loop does the real work
}

bool save(const std::string& file, int step, double x)
{
    const std::string tmp = file + ".tmp";
    {
        std::ofstream out(tmp);
        out.precision(17);
        out << "step " << step << " x " << x << "\n";
        if (!out.flush()) return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, file, ec);        // replaces the old checkpoint in one step
    return !ec;
}

int main(int argc, char** argv)
{
    int steps = 5, every = 10, stepMs = 10;
    std::string ckpt;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a.rfind("steps=", 0) == 0) steps = std::atoi(a.c_str() + 6);
        if (a.rfind("every=", 0) == 0) every = std::atoi(a.c_str() + 6);
        if (a.rfind("step_ms=", 0) == 0) stepMs = std::atoi(a.c_str() + 8);
        if (a.rfind("ckpt=", 0) == 0) ckpt = a.substr(5);
    }
    std::signal(SIGTERM, onTerm);

    int step = 0;
    double x = 1.0;
    if (!ckpt.empty()) {
        std::ifstream in(ckpt);
        std::string w1, w2;
        if (in >> w1 >> step >> w2 >> x && w1 == "step" && w2 == "x") {
            std::printf("worker: resumed from %s at step %d\n", ckpt.c_str(), step);
        } else {
            step = 0;
            x = 1.0;
            std::printf("worker: no checkpoint at %s, starting at step 0\n", ckpt.c_str());
        }
    }
    std::fflush(stdout);
    const int first = step;
    for (; step < steps; ++step) {
        x = 0.5 * x + 1.0 / (step + 1);                              // the "training" step
        std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
        const bool last = step + 1 == steps;
        if (!ckpt.empty() && ((step + 1) % every == 0 || stopRequested || last)) {
            if (!save(ckpt, step + 1, x)) {
                std::printf("worker: cannot write %s\n", ckpt.c_str());
                return 2;
            }
        }
        if (stopRequested && !last) {
            std::printf("worker: SIGTERM received; checkpoint written at step %d (this run did steps %d to %d);"
                        " exiting with 99\n", step + 1, first, step);
            return 99;
        }
    }
    std::printf("worker: finished %d steps (this run did steps %d to %d), x = %.15f\n", steps, first, steps - 1, x);
    return 0;
}
