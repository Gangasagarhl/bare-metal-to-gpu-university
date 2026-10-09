// The "store buffering" litmus test, run for real on two threads many times.
//   thread A: x = 1; r1 = y;        thread B: y = 1; r2 = x;
// Under sequential consistency at least one of r1, r2 must be 1. The program counts how often
// each outcome really happened with three choices of C++ memory order.
// Counts are observations on the machine that ran this program, not guarantees.
#include <atomic>
#include <cstdio>
#include <thread>

constexpr int kRounds = 200'000;

std::atomic<int> x{0};
std::atomic<int> y{0};
std::atomic<int> go{-1};      // round number the workers may start
std::atomic<int> done{0};     // how many workers finished the current round

template <std::memory_order StoreOrder, std::memory_order LoadOrder>
void worker(std::atomic<int>& mine, std::atomic<int>& other, int* results)
{
    for (int round = 0; round < kRounds; ++round) {
        while (go.load(std::memory_order_acquire) != round) {
            // spin until the coordinator opens this round
        }
        mine.store(1, StoreOrder);
        results[round] = other.load(LoadOrder);
        done.fetch_add(1, std::memory_order_acq_rel);
    }
}

template <std::memory_order StoreOrder, std::memory_order LoadOrder>
void experiment(const char* name)
{
    static int r1[kRounds];
    static int r2[kRounds];
    done.store(0);
    go.store(-1);
    std::thread a(worker<StoreOrder, LoadOrder>, std::ref(x), std::ref(y), r1);
    std::thread b(worker<StoreOrder, LoadOrder>, std::ref(y), std::ref(x), r2);
    for (int round = 0; round < kRounds; ++round) {
        x.store(0);
        y.store(0);
        go.store(round, std::memory_order_release);              // open the round
        while (done.load(std::memory_order_acquire) != 2 * (round + 1)) {
            // wait for both workers
        }
    }
    a.join();
    b.join();
    int count[2][2] = {};
    for (int round = 0; round < kRounds; ++round) {
        ++count[r1[round]][r2[round]];
    }
    std::printf("%-8s r1=0 r2=0: %7d   r1=0 r2=1: %7d   r1=1 r2=0: %7d   r1=1 r2=1: %7d\n", name,
                count[0][0], count[0][1], count[1][0], count[1][1]);
}

int main()
{
    std::printf("%d rounds of the store-buffering test per memory order\n", kRounds);
    experiment<std::memory_order_relaxed, std::memory_order_relaxed>("relaxed");
    experiment<std::memory_order_release, std::memory_order_acquire>("rel/acq");
    experiment<std::memory_order_seq_cst, std::memory_order_seq_cst>("seq_cst");
    return 0;
}
