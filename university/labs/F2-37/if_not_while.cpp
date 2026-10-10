// Forensic program (F2-37): "The pass that served an empty plate".
// A hand-written queue whose pop() checks its condition with `if` instead of a loop.
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex printMutex;  // one line of output at a time

void say(const std::string& line)
{
    std::lock_guard<std::mutex> lock(printMutex);
    std::cout << line << std::endl;
}

class PlatePass
{
public:
    void put(const std::string& dish)
    {
        std::lock_guard<std::mutex> lock(m_);
        plates_.push_back(dish);
        cv_.notify_all();  // wake every waiting waiter
    }

    std::string take(int waiterId)
    {
        std::unique_lock<std::mutex> lock(m_);
        ++waiting_;
        if (plates_.empty()) {  // BUG under investigation
            cv_.wait(lock);
        }
        --waiting_;
        if (plates_.empty()) {
            say("waiter " + std::to_string(waiterId) + ": took a plate from an EMPTY pass");
            return "(nothing)";
        }
        std::string dish = plates_.back();
        plates_.pop_back();
        return dish;
    }

    int waiting()
    {
        std::lock_guard<std::mutex> lock(m_);
        return waiting_;
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<std::string> plates_;
    int waiting_ = 0;
};

int main()
{
    PlatePass pass;
    std::vector<std::thread> waiters;
    for (int id = 1; id <= 2; ++id) {
        waiters.emplace_back([&pass, id] {
            const std::string dish = pass.take(id);
            say("waiter " + std::to_string(id) + " serves: " + dish);
        });
    }
    while (pass.waiting() < 2) {  // let both waiters reach the pass first
        std::this_thread::yield();
    }
    say("chef puts one plate of noodles on the pass");
    pass.put("noodles");
    for (std::thread& w : waiters) {
        w.join();
    }
    return 0;
}
