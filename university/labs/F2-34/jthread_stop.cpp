// Listing 3 (F2-34): std::jthread joins itself and can be asked to stop (C++20).
#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>

int main()
{
    int ticks = 0;
    {
        std::jthread timer([&ticks](std::stop_token stop) {
            while (!stop.stop_requested()) {
                ++ticks;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "main: asking the timer to stop\n";
        timer.request_stop();
    }  // ~jthread(): request_stop() (already done) and then join()
    std::cout << "main: timer has finished; it ticked " << (ticks > 0 ? "at least once" : "never")
              << '\n';
    return 0;
}
