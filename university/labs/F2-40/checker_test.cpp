// Listing 2 (F2-40): the curriculum B10 acceptance test "The lock-order checker reports a
// deliberately inverted pair in a test build". The two threads run one after the other,
// so the program never deadlocks; the checker still sees the dangerous order.
#include <iostream>
#include <mutex>
#include <thread>

#include "lock_order_checker.hpp"

CheckedMutex pantry("pantry");
CheckedMutex fridge("fridge");

int main()
{
    std::thread cook1([] {
        std::lock_guard<CheckedMutex> a(pantry);
        std::lock_guard<CheckedMutex> b(fridge);  // records pantry -> fridge
    });
    cook1.join();
    std::thread cook2([] {
        std::lock_guard<CheckedMutex> b(fridge);
        std::lock_guard<CheckedMutex> a(pantry);  // fridge -> pantry: the deliberate inversion
    });
    cook2.join();
    const int found = LockOrderRegistry::instance().inversions();
    std::cout << "inversions reported: " << found << '\n';
    std::cout << (found == 1 ? "PASS: the deliberately inverted pair was reported" : "FAIL") << '\n';
    return found == 1 ? 0 : 1;
}
