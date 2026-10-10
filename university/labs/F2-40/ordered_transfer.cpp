// Listing 1 (F2-40): two ways to take two locks without deadlock, each run with opposite transfers.
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

struct Account
{
    int id;  // a fixed, unique number used to order the locks
    long balance;
    std::mutex m;
};

// Fix 1: always lock the account with the smaller id first (a global lock order).
void transferOrdered(Account& from, Account& to, long amount)
{
    Account& first = from.id < to.id ? from : to;
    Account& second = from.id < to.id ? to : from;
    std::lock_guard<std::mutex> l1(first.m);
    std::lock_guard<std::mutex> l2(second.m);
    from.balance -= amount;
    to.balance += amount;
}

// Fix 2: let std::scoped_lock take both at once with its deadlock-avoidance algorithm.
void transferScoped(Account& from, Account& to, long amount)
{
    std::scoped_lock both(from.m, to.m);
    from.balance -= amount;
    to.balance += amount;
}

template <typename F>
void runBothDirections(const std::string& name, F transfer)
{
    Account a{1, 1000, {}};
    Account b{2, 1000, {}};
    std::thread t1([&] {
        for (int i = 0; i < 100'000; ++i) {
            transfer(a, b, 1);
        }
    });
    std::thread t2([&] {
        for (int i = 0; i < 100'000; ++i) {
            transfer(b, a, 1);
        }
    });
    t1.join();
    t2.join();
    std::cout << name << ": finished, a = " << a.balance << ", b = " << b.balance << '\n';
}

int main()
{
    runBothDirections("ordered by id", transferOrdered);
    runBothDirections("std::scoped_lock", transferScoped);
    return 0;
}
