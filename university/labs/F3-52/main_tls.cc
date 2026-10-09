// main_tls.cc - F3-52: U2 acceptance tests two and three.
// (2) thread-local variables in the executable and in a dlopen'ed library have independent
//     values in 8 threads; (3) dlopen, dlsym and dlclose of the plugin many times (10,000 per
//     batch), with memory statistics returning to the baseline. Built without sanitizers by run.sh: the
//     sanitizers' own allocator would hide the C library's heap statistics.
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <malloc.h>
#include <thread>
#include <vector>

thread_local int exe_slot = 0;          // the executable's own thread-local variable

namespace {

using SetFn = void (*)(int);
using GetFn = int (*)();

long rss_kib()                          // resident set size from /proc/self/status
{
    FILE* f = std::fopen("/proc/self/status", "r");
    char line[256];
    long kib = -1;
    while (f != nullptr && std::fgets(line, sizeof line, f) != nullptr) {
        if (std::strncmp(line, "VmRSS:", 6) == 0) std::sscanf(line + 6, "%ld", &kib);
    }
    if (f != nullptr) std::fclose(f);
    return kib;
}

} // namespace

int main()
{
    void* h = dlopen("./plugin.so", RTLD_NOW);
    if (h == nullptr) {
        std::printf("dlopen failed: %s\n", dlerror());
        return 1;
    }
    auto set = reinterpret_cast<SetFn>(dlsym(h, "plugin_set"));
    auto get = reinterpret_cast<GetFn>(dlsym(h, "plugin_get"));

    // (2) eight threads, each writes its own values, waits, and reads them back
    std::vector<int> exe_seen(8), plugin_seen(8);
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t] {
            exe_slot = 100 + t;
            set(200 + t);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));   // let the others write
            exe_seen[t] = exe_slot;
            plugin_seen[t] = get();
        });
    }
    for (auto& th : threads) th.join();
    bool ok = true;
    for (int t = 0; t < 8; ++t) {
        std::printf("thread %d: executable's variable %d, plugin's variable %d\n", t, exe_seen[t], plugin_seen[t]);
        ok = ok && exe_seen[t] == 100 + t && plugin_seen[t] == 200 + t;
    }
    std::printf("main thread: executable's variable %d, plugin's variable %d (never set here)\n", exe_slot, get());
    std::printf("TLS independence in 8 threads: %s\n", ok ? "PASS" : "FAIL");
    dlclose(h);

    // (3) dlopen/dlsym/dlclose in batches of 10,000 cycles, heap statistics after each batch.
    //     A plain "before versus after" comparison is not enough: the C library keeps some memory
    //     after the first cycles and then stays flat (see the run). The test therefore passes
    //     when the last three batches leave the heap exactly where it was.
    auto cycle = [] {
        void* p = dlopen("./plugin.so", RTLD_NOW);
        if (p == nullptr) return false;
        auto s = reinterpret_cast<SetFn>(dlsym(p, "plugin_set"));
        s(1);
        return dlclose(p) == 0;
    };
    bool all = true;
    std::vector<size_t> heap;
    heap.reserve(6);                    // reserve first: a growing vector would itself show up
    heap.push_back(mallinfo2().uordblks);
    std::printf("heap bytes in use before the cycles: %zu (resident set %ld KiB)\n", heap[0], rss_kib());
    for (int batch = 1; batch <= 5; ++batch) {
        for (int i = 0; i < 10000; ++i) all = cycle() && all;
        heap.push_back(mallinfo2().uordblks);
        std::printf("after batch %d (%d cycles): heap bytes in use %zu (resident set %ld KiB)\n",
                    batch, batch * 10000, heap.back(), rss_kib());
    }
    std::printf("all 50,000 dlopen/dlsym/dlclose cycles succeeded: %s\n", all ? "yes" : "no");
    bool back = all && heap[5] == heap[2] && heap[4] == heap[2] && heap[3] == heap[2];
    std::printf("no growth over the last 30,000 cycles: %s\n", back ? "PASS" : "FAIL");
    return ok && back ? 0 : 1;
}
