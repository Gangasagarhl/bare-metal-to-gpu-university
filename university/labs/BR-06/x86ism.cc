// x86ism.cc - BR-06: "core" code with one x86 instruction inside. It builds and works on a
// PC. run.sh compiles it for AArch64 and RISC-V too, where the build stops.
#include <atomic>

namespace {

inline void cpu_relax()
{
    asm volatile("pause");      // x86 spin-wait hint, written into shared code
}

} // namespace

int wait_for(std::atomic<int>& flag)
{
    int spins = 0;
    while (flag.load(std::memory_order_acquire) == 0) {
        cpu_relax();
        ++spins;
    }
    return spins;
}
