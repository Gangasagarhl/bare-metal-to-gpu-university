// Listing 3 (F2-36): std::shared_mutex lets many readers in at once; a writer gets it alone.
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

class Menu
{
public:
    long price(const std::string& dish) const
    {
        std::shared_lock<std::shared_mutex> reading(mutex_);  // shared ownership
        const auto it = prices_.find(dish);
        return it == prices_.end() ? -1 : it->second;
    }

    void setPrice(const std::string& dish, long cents)
    {
        std::unique_lock<std::shared_mutex> writing(mutex_);  // exclusive ownership
        prices_[dish] = cents;
    }

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, long> prices_{{"soup", 400}, {"rice", 300}};
};

int main()
{
    Menu menu;
    std::vector<std::thread> waiters;
    std::vector<long> lookups(4, 0);
    for (int w = 0; w < 4; ++w) {
        waiters.emplace_back([&menu, &lookups, w] {
            for (int i = 0; i < 50'000; ++i) {
                if (menu.price(i % 2 == 0 ? "soup" : "rice") > 0) {
                    ++lookups[w];
                }
            }
        });
    }
    std::thread manager([&menu] {
        for (int i = 0; i < 1000; ++i) {
            menu.setPrice("soup", 400 + i % 7);
        }
    });
    for (std::thread& w : waiters) {
        w.join();
    }
    manager.join();
    long all = 0;
    for (long n : lookups) {
        all += n;
    }
    std::cout << "successful lookups: " << all << " of 200000\n";
    std::cout << "final soup price: " << menu.price("soup") << " cents\n";
    return 0;
}
