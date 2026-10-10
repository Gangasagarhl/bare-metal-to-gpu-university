// wq_sim.cpp - runs the work-queue model: configuration A, then the same items with
// one thread per item (configuration C), to compare timing and thread count.
#include "wq_model.h"

int main()
{
    using namespace wq_model;
    const Config a = configA();
    simulate(a, 200000, 2);

    Config c = a;   // C: every item gets its own thread, priorities in the same order
    c.title = "C: one thread per item";
    c.queues = {{"t_imu", 10}, {"t_rate", 9}, {"t_att", 8}, {"t_pos", 7}, {"t_batt", 3}, {"t_magcal", 2}};
    for (std::size_t i = 0; i < c.items.size(); ++i) {
        c.items[i].wq = static_cast<int>(i);
    }
    simulate(c, 200000, 2);
    return 0;
}
