// Worked-example check (F2-41): time and efficiency of N tasks on P workers when every task
// does t microseconds of work and costs o microseconds of pool overhead.
#include <iostream>

int main()
{
    const double n = 10000;  // tasks
    const double p = 4;      // workers
    const double o = 5;      // overhead per task, microseconds
    for (double t : {1.0, 10.0, 100.0, 1000.0}) {
        const double serial = n * t;
        const double pooled = n * (t + o) / p;
        std::cout << "task " << t << " us: serial " << serial / 1000 << " ms, pool of 4 " << pooled / 1000
                  << " ms, speedup " << serial / pooled << ", efficiency " << 100 * t / (t + o) << " %\n";
    }
    return 0;
}
