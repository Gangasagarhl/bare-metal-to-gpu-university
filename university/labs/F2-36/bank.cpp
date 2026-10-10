// Listing 1 (F2-36): a mutex protects an invariant: money moves between accounts, the total never changes.
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

class Bank
{
public:
    explicit Bank(int accounts, long start) : balance_(accounts, start) {}

    void transfer(int from, int to, long amount)
    {
        std::lock_guard<std::mutex> guard(mutex_);  // lock now, unlock at the closing brace
        if (balance_[from] >= amount) {
            balance_[from] -= amount;  // between these two lines the invariant is broken...
            balance_[to] += amount;    // ...but no other thread can look: they wait for the lock
        }
    }

    long total()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        long sum = 0;
        for (long b : balance_) {
            sum += b;
        }
        return sum;
    }

private:
    std::mutex mutex_;            // protects balance_
    std::vector<long> balance_;   // invariant: the sum of all balances is constant
};

int main()
{
    Bank bank(8, 1000);
    std::cout << "total before: " << bank.total() << '\n';
    std::vector<std::thread> tellers;
    for (int t = 0; t < 4; ++t) {
        tellers.emplace_back([&bank, t] {
            for (int i = 0; i < 100'000; ++i) {
                const int from = (i + t) % 8;
                const int to = (i * 3 + t + 1) % 8;
                bank.transfer(from, to, 1 + i % 50);
            }
        });
    }
    for (std::thread& teller : tellers) {
        teller.join();
    }
    std::cout << "total after:  " << bank.total() << '\n';
    return 0;
}
