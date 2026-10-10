// BR-01 Listing 8: a CPU model of "the launch returns before the work is done".
// The university's own teaching model, not CUDA: launch() only queues the work, and the
// work runs when synchronize() is called. A real GPU starts the work by itself, so an early
// read there is not guaranteed to see old values, new values or any mix of them.
#include <cstddef>
#include <cstdio>
#include <functional>
#include <utility>
#include <vector>

class StreamModel
{
public:
    void launch(std::function<void()> work)     // queues the work and returns at once
    {
        queue_.push_back(std::move(work));
    }
    void synchronize()                          // runs everything queued, then returns
    {
        for (auto& work : queue_) {
            work();
        }
        queue_.clear();
    }

private:
    std::vector<std::function<void()>> queue_;
};

int main()
{
    const std::size_t n = 8;
    std::vector<float> a(n, 1.0f), b(n, 2.0f), c(n, 0.0f);   // c plays a shared (managed) buffer
    StreamModel stream;

    stream.launch([&] {
        for (std::size_t i = 0; i < n; ++i) {
            c[i] = a[i] + b[i];
        }
    });
    std::printf("read right after launch():  c[1] = %.1f   (too early)\n", c[1]);
    stream.synchronize();
    std::printf("read after synchronize():   c[1] = %.1f   (correct)\n", c[1]);
    return 0;
}
