// Listing 3 (F2-38): which std::atomic<T> are lock-free on this platform, and std::atomic_flag.
#include <atomic>
#include <iostream>

struct Pair
{
    long a;
    long b;
};

struct Triple
{
    long a;
    long b;
    long c;
};

int main()
{
    std::cout << std::boolalpha;
    std::cout << "atomic<bool>   always lock-free: " << std::atomic<bool>::is_always_lock_free << '\n';
    std::cout << "atomic<int>    always lock-free: " << std::atomic<int>::is_always_lock_free << '\n';
    std::cout << "atomic<long>   always lock-free: " << std::atomic<long>::is_always_lock_free << '\n';
    std::cout << "atomic<double> always lock-free: " << std::atomic<double>::is_always_lock_free << '\n';
    std::cout << "atomic<Pair>   (16 bytes) always lock-free: " << std::atomic<Pair>::is_always_lock_free
              << '\n';
    std::cout << "atomic<Triple> (24 bytes) always lock-free: " << std::atomic<Triple>::is_always_lock_free
              << '\n';
    std::atomic_flag flag;  // C++20: starts clear
    std::cout << "atomic_flag: first test_and_set returned " << flag.test_and_set()
              << ", second returned " << flag.test_and_set() << '\n';
    return 0;
}
