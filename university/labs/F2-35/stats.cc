// Forensic program (F2-35): "The revenue that drifts". A kitchen statistics object.
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class KitchenStats
{
public:
    void recordOrder(long priceCents)
    {
        {
            std::lock_guard<std::mutex> guard(mutex_);
            ++orders_;
        }
        revenueCents_ += priceCents;
    }

    void print()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        std::cout << "orders = " << orders_ << ", revenue = " << revenueCents_ / 100 << "."
                  << revenueCents_ % 100 << '\n';
    }

private:
    std::mutex mutex_;
    long orders_ = 0;
    long revenueCents_ = 0;
};

int main()
{
    KitchenStats stats;
    std::vector<std::thread> tills;
    for (int t = 0; t < 4; ++t) {
        tills.emplace_back([&stats] {
            for (int i = 0; i < 100'000; ++i) {
                stats.recordOrder(250);  // every order costs 2.50
            }
        });
    }
    for (std::thread& till : tills) {
        till.join();
    }
    stats.print();
    std::cout << "expected: orders = 400000, revenue = 1000000.0\n";
    return 0;
}
