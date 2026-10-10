// Listing 2 (F2-35): a plain bool used as a "please stop" flag between two threads.
// This is a data race, so the program has undefined behaviour. run.sh builds it twice.
#include <chrono>
#include <iostream>
#include <thread>

bool stopRequested = false;  // shared, NOT atomic, no mutex: the bug
long spins = 0;

void worker()
{
    while (!stopRequested) {
        ++spins;
    }
}

int main()
{
    std::thread t(worker);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stopRequested = true;
    std::cout << "main: stop requested, waiting for the worker" << std::endl;
    t.join();
    std::cout << "main: worker stopped" << std::endl;
    return 0;
}
