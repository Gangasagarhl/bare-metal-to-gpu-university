// F1-48 Listing 2: ask the C++ library about its steady clock, then measure how
// long "sleep for 1 ms" really takes, 20 times. A measurement on ONE machine on
// ONE day; your numbers will differ (that is the point of measuring).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

int main()
{
    using clk = std::chrono::steady_clock;
    std::printf("steady_clock tick period: %lld/%lld s, is_steady=%d\n",
                static_cast<long long>(clk::period::num), static_cast<long long>(clk::period::den),
                clk::is_steady ? 1 : 0);
    std::vector<long long> us;
    for (int i = 0; i < 20; ++i) {
        const auto t0 = clk::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        const auto t1 = clk::now();
        us.push_back(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }
    std::sort(us.begin(), us.end());
    std::printf("sleep_for(1 ms), 20 runs: min %lld us, median %lld us, max %lld us\n", us.front(),
                (us[9] + us[10]) / 2, us.back());
    std::printf("never shorter than asked: %s\n", us.front() >= 1000 ? "yes" : "NO");
    return 0;
}
