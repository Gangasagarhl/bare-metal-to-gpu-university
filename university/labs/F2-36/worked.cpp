// Worked-example check (F2-36): the throughput ceiling a single lock puts on a service.
// Each request spends `outside` ns of work that needs no lock and `inside` ns holding the lock.
#include <algorithm>
#include <iostream>

int main()
{
    const double inside = 200.0;   // ns per request with the lock held
    const double outside = 800.0;  // ns per request without the lock
    const double lockCeiling = 1e9 / inside;  // requests per second the lock can serve at most
    std::cout << "lock ceiling: " << lockCeiling / 1e6 << " million requests/s\n";
    for (int threads = 1; threads <= 8; ++threads) {
        const double noLockLimit = threads * 1e9 / (inside + outside);
        const double bound = std::min(noLockLimit, lockCeiling);
        std::cout << threads << " threads: without the lock " << noLockLimit / 1e6
                  << " M/s, upper bound with the lock " << bound / 1e6 << " M/s\n";
    }
    return 0;
}
