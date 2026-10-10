// ordering_codegen.cc - BR-06: five tiny functions whose machine code run.sh prints for
// x86-64, AArch64 and RISC-V. Same C++, three instruction sets: where does each
// memory order turn into a barrier or a special load/store instruction? The last function
// shows what GCC does when the memory order is not a compile-time constant.
#include <atomic>

std::atomic<int> g_data{0};
std::atomic<int> g_flag{0};

void publish_relaxed(int v)          // producer, no ordering asked for
{
    g_data.store(v, std::memory_order_relaxed);
    g_flag.store(1, std::memory_order_relaxed);
}

void publish_release(int v)          // producer, the flag store is a release
{
    g_data.store(v, std::memory_order_relaxed);
    g_flag.store(1, std::memory_order_release);
}

int consume_acquire()                // consumer, the flag load is an acquire
{
    while (g_flag.load(std::memory_order_acquire) == 0) {
    }
    return g_data.load(std::memory_order_relaxed);
}

int store_then_load_seq_cst()        // SB pattern, sequentially consistent
{
    g_data.store(1, std::memory_order_seq_cst);
    return g_flag.load(std::memory_order_seq_cst);
}

void store_with_runtime_order(int v, std::memory_order order)   // order known only at run time
{
    g_data.store(v, order);
}
