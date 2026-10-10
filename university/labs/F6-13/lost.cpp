// F6-13 Listing 2: lost updates, replayed deterministically.
// K threads each add 1 to a shared counter `per` times. A non-atomic add is three steps
// (load into a register, add, store); a seeded scheduler interleaves those steps.
// An atomic add is one indivisible step. No real threads: a repeatable model of the rule.
#include <cstdio>
#include <random>
#include <vector>

long run(int threads, int per, bool atomic, unsigned seed)
{
    std::mt19937 rng(seed);
    long counter = 0;
    std::vector<int> left(threads, per), stage(threads, 0);
    std::vector<long> reg(threads, 0);
    int active = threads;
    while (active > 0) {
        int t = static_cast<int>(rng() % static_cast<unsigned>(threads));   // scheduler picks a thread
        if (left[t] == 0) { continue; }
        if (atomic) {
            ++counter;                                   // read-modify-write as ONE step
            if (--left[t] == 0) { --active; }
            continue;
        }
        if (stage[t] == 0) { reg[t] = counter; stage[t] = 1; }          // load
        else if (stage[t] == 1) { reg[t] = reg[t] + 1; stage[t] = 2; }  // add
        else {                                                          // store
            counter = reg[t];
            stage[t] = 0;
            if (--left[t] == 0) { --active; }
        }
    }
    return counter;
}

int main()
{
    const int per = 1000;
    std::printf("%-8s %-10s %-12s %-12s %s\n", "threads", "expected", "non-atomic", "atomic", "lost");
    for (int threads : {1, 2, 4, 32, 256}) {
        long expected = static_cast<long>(threads) * per;
        long plain = run(threads, per, false, 2026u);
        long atom = run(threads, per, true, 2026u);
        std::printf("%-8d %-10ld %-12ld %-12ld %ld\n", threads, expected, plain, atom, expected - plain);
    }
    std::printf("same 32 threads, five different schedules (seeds 1..5):");
    for (unsigned s = 1; s <= 5; ++s) { std::printf(" %ld", run(32, per, false, s)); }
    std::printf("\n");
    return 0;
}
