// F9-12 Listing 3 (the fix for the forensic lab): the same camera and detector, but the
// detector always takes the NEWEST frame and drops older ones ("keep latest").
#include <cstdio>
#include <optional>
#include <utility>

int main()
{
    const double period = 1000.0 / 30.0;
    const double work = 40.0;
    std::optional<std::pair<int, double>> latest;  // a queue of length one
    double busyUntil = 0.0;
    int nextFrame = 0, dropped = 0, done = 0;
    std::printf("frame  captured_ms  result_ms  age_of_result_ms  dropped_so_far\n");
    for (double now = 0.0; now <= 6000.0; now += 1.0) {
        while (nextFrame * period <= now) {
            if (latest) ++dropped;  // an unprocessed older frame is replaced
            latest = std::pair<int, double>{nextFrame, nextFrame * period};
            ++nextFrame;
        }
        if (now >= busyUntil && latest) {
            const auto [id, captured] = *latest;
            latest.reset();
            busyUntil = now + work;
            if (done % 12 == 0) {
                std::printf("%5d %12.1f %10.1f %17.1f %15d\n", id, captured, busyUntil,
                            busyUntil - captured, dropped);
            }
            ++done;
        }
    }
    return 0;
}
