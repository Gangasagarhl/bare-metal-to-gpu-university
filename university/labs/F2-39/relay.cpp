// Listing 2 (F2-39): happens-before is transitive. Three cooks pass a dish along with two flags.
#include <atomic>
#include <iostream>
#include <thread>

int sauce = 0;  // plain data written by the first cook only
std::atomic<int> flag1{0};
std::atomic<int> flag2{0};

int main()
{
    std::thread first([] {
        sauce = 42;                                  // A
        flag1.store(1, std::memory_order_release);   // B: release
    });
    std::thread second([] {
        while (flag1.load(std::memory_order_acquire) != 1) {  // C: acquire, reads B
        }
        flag2.store(1, std::memory_order_release);   // D: release
    });
    std::thread third([] {
        while (flag2.load(std::memory_order_acquire) != 1) {  // E: acquire, reads D
        }
        std::cout << "third cook sees sauce = " << sauce << '\n';  // F: A happens-before F
    });
    first.join();
    second.join();
    third.join();
    return 0;
}
