// Worked-example check (F2-34): slice boundaries n*t/T and the per-slice sums of 1..n.
#include <cstddef>
#include <iostream>

int main()
{
    const std::size_t n = 10;
    const std::size_t numThreads = 4;
    for (std::size_t t = 0; t < numThreads; ++t) {
        const std::size_t begin = n * t / numThreads;
        const std::size_t end = n * (t + 1) / numThreads;
        long sum = 0;
        for (std::size_t i = begin; i < end; ++i) {
            sum += static_cast<long>(i) + 1;  // element i holds the value i + 1
        }
        std::cout << "thread " << t << ": indices [" << begin << ", " << end << ") -> "
                  << end - begin << " elements, sum " << sum << '\n';
    }
    return 0;
}
