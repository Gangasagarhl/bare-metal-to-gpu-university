// F8-05 forensic evidence: "right on Monday, wrong on Tuesday".
// The same ring as Listing 1, but a colleague replaced the mailbox by a "faster" one-slot
// handoff: a plain bool flag and a buffer. Two planted bugs: the flag is not atomic (no
// happens-before between the threads), and the sender raises the flag BEFORE it copies the
// data. Built by run.sh with ThreadSanitizer (-fsanitize=thread), which reports the race.
#include <cstdio>
#include <thread>
#include <vector>

struct Slot
{
    std::vector<int> data;
    bool ready = false;          // BUG 1: a plain bool used to synchronise two threads
};

void send(Slot& s, const int* from, std::size_t len)
{
    while (s.ready) {            // wait until the receiver emptied the slot
        std::this_thread::yield();
    }
    s.ready = true;              // BUG 2: announce first ...
    s.data.assign(from, from + len);   // ... copy second
}

std::vector<int> receive(Slot& s)
{
    while (!s.ready) {
        std::this_thread::yield();
    }
    std::vector<int> got = s.data;
    s.ready = false;
    return got;
}

int main()
{
    const std::size_t n = 4, count = 4096, seg = count / n;
    int wrongRuns = 0;
    for (int run = 0; run < 20; ++run) {
        std::vector<std::vector<int>> buf(n, std::vector<int>(count));
        for (std::size_t r = 0; r < n; ++r) {
            for (std::size_t i = 0; i < count; ++i) {
                buf[r][i] = static_cast<int>((r + 1) * (i % 100));
            }
        }
        std::vector<Slot> slot(n);                      // slot[r] is rank r's inbox
        auto rank = [&](std::size_t r) {
            for (int phase = 0; phase < 2; ++phase) {
                for (std::size_t step = 0; step + 1 < n; ++step) {
                    const std::size_t s = (r + 2 * n - step + phase) % n;
                    const std::size_t d = (r + 2 * n - step - 1 + phase) % n;
                    send(slot[(r + 1) % n], buf[r].data() + s * seg, seg);
                    const std::vector<int> got = receive(slot[r]);
                    for (std::size_t i = 0; i < got.size(); ++i) {
                        buf[r][d * seg + i] = phase == 0 ? buf[r][d * seg + i] + got[i] : got[i];
                    }
                }
            }
        };
        std::vector<std::thread> t;
        for (std::size_t r = 0; r < n; ++r) {
            t.emplace_back(rank, r);
        }
        for (auto& th : t) {
            th.join();
        }
        bool ok = true;
        for (std::size_t r = 0; r < n; ++r) {
            for (std::size_t i = 0; i < count; ++i) {
                ok = ok && buf[r][i] == static_cast<int>(10 * (i % 100));
            }
        }
        wrongRuns += ok ? 0 : 1;
    }
    std::printf("20 runs of a 4-rank ring all-reduce: %d gave a wrong result\n", wrongRuns);
    return 0;
}
