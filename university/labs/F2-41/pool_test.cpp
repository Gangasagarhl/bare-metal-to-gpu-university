// Listing 2 (F2-41): tests for ThreadPool. Each test prints PASS or FAIL; the program
// returns 1 if any test failed. run_lab.sh runs it with AddressSanitizer and UBSan;
// run.sh runs it again with ThreadSanitizer.
#include <atomic>
#include <iostream>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "thread_pool.hpp"

int failures = 0;

void check(bool ok, const std::string& name)
{
    std::cout << (ok ? "PASS  " : "FAIL  ") << name << '\n';
    if (!ok) {
        ++failures;
    }
}

int main()
{
    {  // 1. every task runs once and its result arrives
        ThreadPool pool(4);
        std::vector<std::future<long>> results;
        for (long i = 0; i < 1000; ++i) {
            results.push_back(pool.submit([i] { return i * i; }));
        }
        long sum = 0;
        for (auto& r : results) {
            sum += r.get();
        }
        check(sum == 332'833'500, "1000 tasks, sum of squares 0..999 = 332833500");
    }
    {  // 2. an exception thrown by a task reaches the caller of get()
        ThreadPool pool(2);
        auto f = pool.submit([]() -> int { throw std::runtime_error("burnt"); });
        bool caught = false;
        try {
            f.get();
        } catch (const std::runtime_error& e) {
            caught = std::string(e.what()) == "burnt";
        }
        check(caught, "exception propagates through the future");
    }
    {  // 3. the destructor finishes queued tasks before joining
        std::atomic<int> done{0};
        {
            ThreadPool pool(1);
            for (int i = 0; i < 100; ++i) {
                pool.submit([&done] { done.fetch_add(1); });
            }
        }  // ~ThreadPool here
        check(done.load() == 100, "destructor drains the queue (100 of 100 tasks ran)");
    }
    {  // 4. many threads may submit at the same time
        ThreadPool pool(3);
        std::atomic<int> ran{0};
        std::vector<std::thread> clients;
        std::vector<std::vector<std::future<void>>> futures(4);
        for (int c = 0; c < 4; ++c) {
            clients.emplace_back([&pool, &ran, &futures, c] {
                for (int i = 0; i < 250; ++i) {
                    futures[c].push_back(pool.submit([&ran] { ran.fetch_add(1); }));
                }
            });
        }
        for (auto& t : clients) {
            t.join();
        }
        for (auto& fs : futures) {
            for (auto& f : fs) {
                f.get();
            }
        }
        check(ran.load() == 1000, "4 client threads x 250 submissions = 1000 runs");
    }
    {  // 5. tasks run on the pool's threads, and on no more threads than the pool has
        ThreadPool pool(2);
        std::mutex m;
        std::set<std::thread::id> ids;
        std::vector<std::future<void>> fs;
        for (int i = 0; i < 200; ++i) {
            fs.push_back(pool.submit([&m, &ids] {
                std::lock_guard<std::mutex> g(m);
                ids.insert(std::this_thread::get_id());
            }));
        }
        for (auto& f : fs) {
            f.get();
        }
        check(ids.size() >= 1 && ids.size() <= 2 && ids.count(std::this_thread::get_id()) == 0,
              "tasks ran on 1 or 2 pool threads, never on main");
    }
    {  // 6. a pool with zero workers is refused
        bool refused = false;
        try {
            ThreadPool pool(0);
        } catch (const std::invalid_argument&) {
            refused = true;
        }
        check(refused, "ThreadPool(0) throws std::invalid_argument");
    }
    std::cout << (failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << '\n';
    return failures == 0 ? 0 : 1;
}
