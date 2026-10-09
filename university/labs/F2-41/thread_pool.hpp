// thread_pool.hpp (F2-41): a fixed-size thread pool. submit() puts a task on a queue and
// returns a std::future for its result. The destructor lets the workers finish every task
// already queued, then joins them.
#pragma once
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class ThreadPool
{
public:
    explicit ThreadPool(std::size_t workers)
    {
        if (workers == 0) {
            throw std::invalid_argument("a pool needs at least one worker");
        }
        for (std::size_t i = 0; i < workers; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        wake_.notify_all();
        for (std::thread& w : workers_) {
            w.join();
        }
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
    {
        using R = std::invoke_result_t<F, Args...>;
        // packaged_task is move-only; std::function needs a copyable target, so share it.
        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<R> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                throw std::runtime_error("submit() on a pool that is shutting down");
            }
            queue_.emplace_back([task] { (*task)(); });
        }
        wake_.notify_one();
        return result;
    }

    std::size_t size() const
    {
        return workers_.size();
    }

    std::size_t queued() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    void workerLoop()
    {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
                if (queue_.empty()) {
                    return;  // stopping and nothing left to do
                }
                job = std::move(queue_.front());
                queue_.pop_front();
            }
            job();  // run outside the lock; a packaged_task stores any exception in its future
        }
    }

    mutable std::mutex mutex_;                  // protects queue_ and stopping_
    std::condition_variable wake_;              // "there is work, or we are stopping"
    std::deque<std::function<void()>> queue_;
    bool stopping_ = false;
    std::vector<std::thread> workers_;          // declared last: started after the members above exist
};
