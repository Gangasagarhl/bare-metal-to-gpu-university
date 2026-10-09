// Listing 2 (F2-36): what happens to a lock when an exception leaves the critical section.
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

std::mutex manualMutex;
std::mutex guardedMutex;

void manualVersion()
{
    manualMutex.lock();
    throw std::runtime_error("out of rice");  // unlock() below is never reached
    manualMutex.unlock();
}

void guardedVersion()
{
    std::lock_guard<std::mutex> guard(guardedMutex);
    throw std::runtime_error("out of rice");  // ~lock_guard() runs during stack unwinding
}

// Asks from another thread whether the mutex is free (try_lock never blocks).
bool isFreeSeenFromAnotherThread(std::mutex& m)
{
    bool free = false;
    std::thread probe([&m, &free] {
        if (m.try_lock()) {
            free = true;
            m.unlock();
        }
    });
    probe.join();
    return free;
}

int main()
{
    try {
        manualVersion();
    } catch (const std::exception& e) {
        std::cout << "manualVersion threw: " << e.what() << '\n';
    }
    try {
        guardedVersion();
    } catch (const std::exception& e) {
        std::cout << "guardedVersion threw: " << e.what() << '\n';
    }
    std::cout << "manualMutex is " << (isFreeSeenFromAnotherThread(manualMutex) ? "free" : "STILL LOCKED")
              << '\n';
    std::cout << "guardedMutex is " << (isFreeSeenFromAnotherThread(guardedMutex) ? "free" : "STILL LOCKED")
              << '\n';
    manualMutex.unlock();  // tidy up: main still owns it after the exception
    return 0;
}
