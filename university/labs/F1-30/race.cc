// F1-30 forensic evidence: Tomás's hit counter. Two threads each count 100000 events into
// one shared int. Built by run.sh twice: normally (-O0) and with ThreadSanitizer.
#include <cstdio>
#include <thread>

int hits = 0;   // shared by both threads, with no atomic and no lock

void countEvents()
{
    for (int i = 0; i < 100000; ++i) {
        ++hits;
    }
}

int main()
{
    std::thread a(countEvents);
    std::thread b(countEvents);
    a.join();
    b.join();
    std::printf("hits = %d (expected 200000)\n", hits);
    return 0;
}
