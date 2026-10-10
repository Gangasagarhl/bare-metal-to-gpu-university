// F9-20 Listing 4: the position every 0.1 s for experiments 2, 3 and 7 of Listing 2
// (used for Figure 2), recorded through the trace argument of simulate().
#include "cart_sim.hpp"
#include <format>
#include <iostream>
#include <vector>

int main()
{
    std::vector<double> e2;
    std::vector<double> e3;
    std::vector<double> e7;
    simulate({8.0, 0.0, 4.8}, 6.0, &e2);
    simulate({8.0, 4.0, 4.8}, 6.0, &e3);
    simulate({8.0, 1.75, 5.5}, 6.0, &e7);
    std::cout << "  t (s)   exp 2 (PD)   exp 3 (PID, Ki=4)   exp 7 (PID, Ki=1.75)\n";
    for (std::size_t i = 0; i < e2.size(); i += 2) {
        std::cout << std::format("{:>7.1f} {:>12.3f} {:>19.3f} {:>22.3f}\n", i * 0.1, e2[i], e3[i],
                                 e7[i]);
    }
    return 0;
}
