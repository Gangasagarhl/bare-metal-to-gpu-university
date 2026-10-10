// F1-26 Listing 3 (forensic evidence): how long do N instructions take on a single-cycle
// machine and on a five-stage pipeline, given the delay of each stage? Delays are read from
// standard input in made-up "delay units" (du); they describe no real chip.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> names;
    std::vector<int> delay;
    int latch = 0;  // extra delay of the pipeline register between two stages
    std::string name;
    int d = 0;
    while (std::cin >> name >> d) {
        if (name == "latch") {
            latch = d;
        } else {
            names.push_back(name);
            delay.push_back(d);
        }
    }
    int sum = 0, slowest = 0;
    for (std::size_t i = 0; i < delay.size(); ++i) {
        std::printf("stage %-4s %4d du\n", names[i].c_str(), delay[i]);
        sum += delay[i];
        slowest = std::max(slowest, delay[i]);
    }
    const int single = sum;               // one long cycle must fit every stage in a row
    const int piped = slowest + latch;    // the clock must fit the slowest stage + its latch
    const long stages = static_cast<long>(delay.size());
    std::printf("single-cycle clock period %d du; pipelined clock period %d du (slowest stage "
                "%d + latch %d)\n", single, piped, slowest, latch);
    std::printf("%10s %16s %16s %9s\n", "N instr", "single-cycle du", "pipelined du", "speed-up");
    for (const long n : {1L, 5L, 10L, 100L, 1000L, 1000000L}) {
        const long t1 = n * single;
        const long t2 = (stages + n - 1) * piped;  // fill the pipe, then one result per cycle
        std::printf("%10ld %16ld %16ld %9.2f\n", n, t1, t2, static_cast<double>(t1) / t2);
    }
    return 0;
}
