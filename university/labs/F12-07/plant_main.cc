// plant_main.cc - the simulated world as its own program. At every control tick it sends the
// reading to the controller process through one pipe and waits for the command on another
// (lock-step: simulated time stops while it waits). Logs one line per tick.
//   plant_main <pipe to controller> <pipe from controller>
#include "wallsim.hpp"

#include <cmath>
#include <cstdio>

namespace {

struct RemoteController {
    std::FILE* to;
    std::FILE* from;
    double command(const wallsim::Reading& r, double)
    {
        std::fprintf(to, "R %ld\n", std::lround(std::max(0.0, r.value_m) * 1000.0));
        std::fflush(to);
        double cmd = 0.0;
        if (std::fscanf(from, " C %lf", &cmd) != 1) {
            std::fprintf(stderr, "plant: no command from controller, stopping the robot\n");
            return 0.0;
        }
        return cmd;
    }
};

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: plant_main <to-controller> <from-controller>\n");
        return 2;
    }
    std::FILE* to = std::fopen(argv[1], "w");     // opened first: the controller reads it first
    std::FILE* from = std::fopen(argv[2], "r");
    if (to == nullptr || from == nullptr) {
        std::perror("plant: open pipe");
        return 2;
    }
    wallsim::World w;
    w.start_d = 3.0;
    w.latency_s = 0.3;
    w.noise_m = 0.003;
    w.seed = 7;
    int k = 0;
    const auto o = wallsim::run(w, RemoteController{to, from}, [&](const wallsim::Tick& t) {
        if (k++ % 10 == 0 && t.t < 5.0) {
            std::printf("t=%5.2f range=%7.4f cmd=%.6f true_d=%.4f\n", t.t, t.reading, t.cmd,
                        t.true_d);
        }
    });
    std::fprintf(to, "Q\n");
    std::fflush(to);
    std::printf("closest approach %.4f m, requirements %s\n", o.min_d,
                wallsim::meets_requirements(o) ? "met" : "NOT met");
    std::fclose(to);
    std::fclose(from);
    return 0;
}
