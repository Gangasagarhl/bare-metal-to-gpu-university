// Listing 1 (F2-37): why wait() needs a predicate. The notification is sent BEFORE the
// waiter starts waiting; a condition variable does not remember it.
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

std::mutex m;
std::condition_variable cv;
bool soupReady = false;  // the shared state the condition is about, protected by m

int main()
{
    {
        std::lock_guard<std::mutex> lock(m);
        soupReady = true;
    }
    cv.notify_one();  // nobody is waiting yet: this wake-up is lost

    std::thread noPredicate([] {
        std::unique_lock<std::mutex> lock(m);
        const auto r = cv.wait_for(lock, std::chrono::milliseconds(300));  // waits for a signal only
        std::cout << "without predicate: "
                  << (r == std::cv_status::timeout ? "timed out after 300 ms (the wake-up was lost)"
                                                   : "woke up")
                  << '\n';
    });
    noPredicate.join();

    std::thread withPredicate([] {
        std::unique_lock<std::mutex> lock(m);
        const bool ok = cv.wait_for(lock, std::chrono::milliseconds(300), [] { return soupReady; });
        std::cout << "with predicate:    " << (ok ? "saw soupReady == true at once" : "timed out") << '\n';
    });
    withPredicate.join();
    return 0;
}
