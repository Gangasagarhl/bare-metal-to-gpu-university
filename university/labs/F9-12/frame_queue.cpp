// F9-12 forensic generator: a camera delivers a frame every 33.3 ms; an obstacle
// detector takes the oldest waiting frame from a queue and needs 40 ms per frame.
// The log shows, per processed frame, when it was captured and when its result was ready.
// Timings are PRETEND values; the queue policy is the fault (see the key).
#include <cstdio>
#include <deque>
#include <utility>

int main()
{
    const double period = 1000.0 / 30.0;  // ms between frames
    const double work = 40.0;             // ms the detector needs per frame
    std::deque<std::pair<int, double>> queue;  // (frame id, capture time)
    double busyUntil = 0.0;
    int nextFrame = 0;
    std::printf("frame  captured_ms  result_ms  age_of_result_ms  waiting_in_queue\n");
    for (double now = 0.0; now <= 6000.0; now += 1.0) {
        while (nextFrame * period <= now) {
            queue.push_back({nextFrame, nextFrame * period});
            ++nextFrame;
        }
        if (now >= busyUntil && !queue.empty()) {
            const auto [id, captured] = queue.front();
            queue.pop_front();
            busyUntil = now + work;
            if (id % 15 == 0) {
                std::printf("%5d %12.1f %10.1f %17.1f %17zu\n", id, captured, busyUntil,
                            busyUntil - captured, queue.size());
            }
        }
    }
    return 0;
}
