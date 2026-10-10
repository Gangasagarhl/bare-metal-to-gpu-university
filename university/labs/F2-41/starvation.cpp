// Forensic program (F2-41): "The report that never finished". Each report task splits its
// work into a sub-task on the SAME pool and waits for it. (wait_for with a time limit is used
// only so that this evidence program ends; the real service used get() and hung forever.)
#include <chrono>
#include <future>
#include <iostream>
#include <string>

#include "thread_pool.hpp"

int main()
{
    ThreadPool pool(2);
    std::vector<std::future<std::string>> reports;
    for (int r = 1; r <= 2; ++r) {
        reports.push_back(pool.submit([&pool, r]() -> std::string {
            std::future<int> part = pool.submit([r] { return r * 100; });  // sub-task, same pool
            if (part.wait_for(std::chrono::milliseconds(500)) == std::future_status::timeout) {
                return "report " + std::to_string(r) + ": sub-task not started after 500 ms (queued tasks: " +
                       std::to_string(pool.queued()) + ")";
            }
            return "report " + std::to_string(r) + ": total " + std::to_string(part.get());
        }));
    }
    for (auto& rep : reports) {
        std::cout << rep.get() << std::endl;
    }
    std::cout << "pool size: " << pool.size() << std::endl;
    return 0;
}
