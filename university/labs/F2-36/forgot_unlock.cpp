// Forensic program (F2-36): "The till that froze after a bad order".
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

std::timed_mutex ticketMutex;
int nextTicket = 1;

// Returns the ticket number, or 0 if the order is rejected.
int takeOrder(const std::string& dish, int quantity)
{
    ticketMutex.lock();
    if (quantity <= 0) {
        std::cout << "[till] rejected order: " << dish << " x" << quantity << std::endl;
        return 0;
    }
    const int ticket = nextTicket++;
    ticketMutex.unlock();
    std::cout << "[till] ticket " << ticket << ": " << dish << " x" << quantity << std::endl;
    return ticket;
}

// The kitchen screen refreshes by peeking at the next ticket number.
void kitchenScreen()
{
    for (int refresh = 1; refresh <= 3; ++refresh) {
        if (ticketMutex.try_lock_for(std::chrono::milliseconds(200))) {
            std::cout << "[screen] refresh " << refresh << ": next ticket " << nextTicket << std::endl;
            ticketMutex.unlock();
        } else {
            std::cout << "[screen] refresh " << refresh << ": could not get ticketMutex within 200 ms"
                      << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main()
{
    takeOrder("soup", 2);
    takeOrder("rice", 1);
    std::thread screen1(kitchenScreen);
    screen1.join();
    takeOrder("tea", 0);
    std::thread screen2(kitchenScreen);
    screen2.join();
    std::cout << "[main] closing for the night" << std::endl;
    ticketMutex.unlock();  // lab clean-up only: main still owns the mutex (see the answer key)
    return 0;
}
