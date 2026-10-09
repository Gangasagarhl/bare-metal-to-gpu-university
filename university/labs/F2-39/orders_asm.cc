// Listing 4 (F2-39): one store and one load of each memory order, compiled (not run) for
// x86-64 and for AArch64 by run.sh, to see which instructions each order becomes.
#include <atomic>

std::atomic<int> flag{0};

void storeRelaxed() { flag.store(1, std::memory_order_relaxed); }
void storeRelease() { flag.store(1, std::memory_order_release); }
void storeSeqCst() { flag.store(1, std::memory_order_seq_cst); }
int loadRelaxed() { return flag.load(std::memory_order_relaxed); }
int loadAcquire() { return flag.load(std::memory_order_acquire); }
int loadSeqCst() { return flag.load(std::memory_order_seq_cst); }
void fenceSeqCst() { std::atomic_thread_fence(std::memory_order_seq_cst); }
