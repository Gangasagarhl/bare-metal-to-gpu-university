// Listing 1 (F2-35): two cashiers sell tickets and both add to one plain counter.
// A data race: run.sh builds it without a checker (three runs) and with ThreadSanitizer.
#include <iostream>
#include <thread>

long sold = 0;  // shared by both threads, nothing protects it

void sell(int count)
{
    for (int i = 0; i < count; ++i) {
        ++sold;  // load, add one, store: three steps another thread can interleave
    }
}

int main()
{
    std::thread cashierA(sell, 1'000'000);
    std::thread cashierB(sell, 1'000'000);
    cashierA.join();
    cashierB.join();
    std::cout << "sold = " << sold << " (expected 2000000)" << std::endl;
    return 0;
}
