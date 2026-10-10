// Listing 3 (F2-35): the same program with the counter made atomic. No data race remains.
#include <atomic>
#include <iostream>
#include <thread>

std::atomic<long> sold{0};  // every ++ is now one indivisible read-modify-write (F2-38)

void sell(int count)
{
    for (int i = 0; i < count; ++i) {
        ++sold;
    }
}

int main()
{
    std::thread cashierA(sell, 1'000'000);
    std::thread cashierB(sell, 1'000'000);
    cashierA.join();
    cashierB.join();
    std::cout << "sold = " << sold.load() << " (expected 2000000)" << std::endl;
    return 0;
}
