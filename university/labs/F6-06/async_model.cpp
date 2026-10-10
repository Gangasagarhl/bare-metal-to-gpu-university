// F6-06 Listing 5: a CPU model of an asynchronous launch (the university's own model,
// not CUDA). "launch" starts the work on another CPU thread and returns at once, like a
// kernel launch; "wait" is the model's synchronisation. Two ways to time it are compared.
#include <chrono>
#include <cstdio>
#include <future>
#include <vector>

using Clock = std::chrono::steady_clock;

static double msSince(Clock::time_point t0)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

static void work(std::vector<float>& y)            // the "kernel"
{
    for (float& v : y) { v = v * 0.5f + 1.0f; }
}

int main()
{
    std::printf("%10s %22s %22s\n", "elements", "timed without wait ms", "timed with wait ms");
    for (std::size_t n : {std::size_t{1} << 18, std::size_t{1} << 20, std::size_t{1} << 22}) {
        std::vector<float> y(n, 1.0f);
        const auto t0 = Clock::now();
        std::future<void> done = std::async(std::launch::async, work, std::ref(y));  // "launch"
        const double noWait = msSince(t0);        // the wrong way: stop the clock now
        done.wait();                              // the model's synchronisation
        const double withWait = msSince(t0);      // the right way for a host clock
        std::printf("%10zu %22.3f %22.3f\n", n, noWait, withWait);
    }
    return 0;
}
