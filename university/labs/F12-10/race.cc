// race.cc - a test that is flaky because the code under test has a data race: two threads
// count events into one plain int. Built twice by run.sh: normally (run 20 times, the result
// varies) and with ThreadSanitizer (which reports the race on the first run).
#include <cstdio>
#include <thread>

namespace {

int g_events = 0;  // planted bug: shared by two threads without a lock or an atomic

void count_events(int n)
{
    for (int i = 0; i < n; ++i) {
        g_events = g_events + 1;
    }
}

}  // namespace

int main()
{
    std::thread a(count_events, 100000);
    std::thread b(count_events, 100000);
    a.join();
    b.join();
    const bool ok = g_events == 200000;
    std::printf("race test: counted %d of 200000: %s\n", g_events, ok ? "pass" : "FAIL");
    return ok ? 0 : 1;
}
