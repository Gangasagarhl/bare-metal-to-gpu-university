// Listing 1 (F2-41): tasks with results: std::async, std::promise and std::packaged_task.
// A future is the receipt; get() waits for the result (or re-throws the task's exception).
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

int slowSquare(int x)
{
    return x * x;
}

int checkedRoot(int x)
{
    if (x < 0) {
        throw std::domain_error("negative input");
    }
    int r = 0;
    while ((r + 1) * (r + 1) <= x) {
        ++r;
    }
    return r;
}

int main()
{
    // 1. std::async with launch::async: runs the function on a new thread
    std::future<int> a = std::async(std::launch::async, slowSquare, 12);
    std::cout << "async result: " << a.get() << '\n';

    // 2. std::promise: one thread promises a value, another waits on the matching future
    std::promise<int> promise;
    std::future<int> fromPromise = promise.get_future();
    std::thread producer([&promise] { promise.set_value(7); });
    std::cout << "promise delivered: " << fromPromise.get() << '\n';
    producer.join();

    // 3. std::packaged_task: wraps a function so that calling it fills a future
    std::packaged_task<int(int)> task(checkedRoot);
    std::future<int> fromTask = task.get_future();
    std::thread worker(std::move(task), 50);
    std::cout << "packaged_task result: " << fromTask.get() << '\n';
    worker.join();

    // 4. exceptions travel through the future to the thread that calls get()
    std::future<int> bad = std::async(std::launch::async, checkedRoot, -4);
    try {
        bad.get();
    } catch (const std::domain_error& e) {
        std::cout << "exception from the task: " << e.what() << '\n';
    }
    return 0;
}
