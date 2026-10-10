// Forensic program (F2-39): "Works on the laptop, garbage on the board".
// A settings thread builds a new configuration and publishes a pointer to it;
// the control loop picks up the pointer and reads the settings.
#include <atomic>
#include <iostream>
#include <thread>

struct Config
{
    int speedLimit = 0;
    int maxTemperature = 0;
};

Config storage;
std::atomic<Config*> current{nullptr};

void settingsThread()
{
    storage.speedLimit = 30;
    storage.maxTemperature = 85;
    current.store(&storage, std::memory_order_relaxed);
}

void controlLoop()
{
    Config* c = nullptr;
    while ((c = current.load(std::memory_order_relaxed)) == nullptr) {
    }
    std::cout << "control loop: speed limit " << c->speedLimit << ", max temperature "
              << c->maxTemperature << '\n';
}

int main()
{
    std::thread control(controlLoop);
    std::thread settings(settingsThread);
    settings.join();
    control.join();
    return 0;
}
