// Listing 3 (F2-40): the same inverted pair with plain std::mutex, for ThreadSanitizer.
// The threads run one after the other: no deadlock happens in this run.
#include <iostream>
#include <mutex>
#include <thread>

std::mutex pantry;
std::mutex fridge;

int main()
{
    std::thread cook1([] {
        std::lock_guard<std::mutex> a(pantry);
        std::lock_guard<std::mutex> b(fridge);
    });
    cook1.join();
    std::thread cook2([] {
        std::lock_guard<std::mutex> b(fridge);
        std::lock_guard<std::mutex> a(pantry);
    });
    cook2.join();
    std::cout << "both cooks finished (no deadlock happened in this run)\n";
    return 0;
}
