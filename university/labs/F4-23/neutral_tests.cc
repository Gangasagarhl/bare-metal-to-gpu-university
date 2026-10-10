// neutral_tests.cc - F4-23: arch-neutral kernel tests (the B1-style checks of this course).
// Rules this file obeys (curriculum 10.2): no inline assembly, no fixed page or cache-line
// size, every multi-byte external value read with an explicit byte order, atomics only
// through std::atomic, no assumption about pointer width beyond what it prints.
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "fdt.h"
#include "kformat.h"
#include "kprint.h"
#include "neutral_tests.h"

namespace {

int g_pass = 0, g_fail = 0;

void check(bool ok, const char* name)
{
    kprintf("  %s %s\n", ok ? "ok  " : "FAIL", name);
    ok ? ++g_pass : ++g_fail;
}

bool same(const char* a, const char* b)
{
    return fdt::streq(a, b);
}

// A global object with a constructor: proves the port runs .init_array before the tests.
struct CtorProbe {
    int value;
    CtorProbe() : value(402) {}
};
CtorProbe g_probe;
uint64_t g_zero[8];   // .bss: must read as zero

// A tiny frame allocator over a fake region: page size comes from the arch layer.
class FrameBitmap {
public:
    FrameBitmap(uint64_t base, uint64_t page) : base_(base), page_(page) {}
    uint64_t alloc()
    {
        for (unsigned i = 0; i < kFrames; ++i) {
            if (!(bits_ & (uint64_t{1} << i))) {
                bits_ |= uint64_t{1} << i;
                return base_ + i * page_;
            }
        }
        return 0;
    }
    void free(uint64_t addr) { bits_ &= ~(uint64_t{1} << ((addr - base_) / page_)); }

private:
    static constexpr unsigned kFrames = 64;
    uint64_t base_, page_;
    uint64_t bits_ = 0;
};

} // namespace

int run_neutral_tests(const NeutralEnv& env)
{
    g_pass = g_fail = 0;
    kprintf("neutral tests on %s: sizeof(void*) %u, sizeof(long) %u, page size %lu\n",
            env.arch_name, static_cast<unsigned>(sizeof(void*)), static_cast<unsigned>(sizeof(long)),
            static_cast<unsigned long>(env.page_size));

    // 1. C++ runtime: constructors ran, .bss is zero
    bool zero = true;
    for (uint64_t v : g_zero) {
        zero = zero && v == 0;
    }
    check(g_probe.value == 402, "global constructor ran before the tests");
    check(zero, ".bss reads as zero");

    // 2. formatting: the same line F3-18's B1 kernel printed on x86-64
    char buf[96];
    ksnprintf(buf, sizeof buf, "[%5d] [%05u] [%x] [%lx] [%s] [%c] [%%]", -42, 99u, 0xbeefu,
              0xffffffff80000000ul, "str", 'k');
    check(same(buf, "[  -42] [00099] [beef] [ffffffff80000000] [str] [k] [%]"),
          "kformat output identical to the x86-64 B1 line");

    // 3. byte order: explicit big-endian reads give the same value on every CPU
    const uint8_t bytes[4] = {0x11, 0x22, 0x33, 0x44};
    uint32_t native = 0;
    for (int i = 3; i >= 0; --i) {
        native = (native << 8) | bytes[i];   // what a little-endian load would give
    }
    uint32_t probe = 1;
    bool little = *reinterpret_cast<const uint8_t*>(&probe) == 1;
    kprintf("  info this CPU is %s-endian; be32(11 22 33 44) = 0x%x\n", little ? "little" : "big",
            fdt::be32(bytes));
    check(fdt::be32(bytes) == 0x11223344u && native == 0x44332211u, "explicit byte-order reads");

    // 4. pointer width: a pointer survives a round trip through uintptr_t
    int local = 7;
    auto as_int = reinterpret_cast<uintptr_t>(&local);
    check(*reinterpret_cast<int*>(as_int) == 7 && sizeof(uintptr_t) == sizeof(void*),
          "pointers round-trip through uintptr_t");

    // 5. page size from the arch layer
    FrameBitmap fb(0x100000ull * env.page_size, env.page_size);
    uint64_t a = fb.alloc(), b = fb.alloc(), c = fb.alloc();
    fb.free(b);
    uint64_t d = fb.alloc();
    check(b - a == env.page_size && c - b == env.page_size && d == b && a % env.page_size == 0,
          "frame allocator uses the arch page size");

    // 6. atomics through std::atomic (lowered to the CPU's own atomic instructions)
    std::atomic<uint64_t> counter{0};
    for (int i = 0; i < 1000; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
    uint64_t expected = 1000;
    bool swapped = counter.compare_exchange_strong(expected, 5000, std::memory_order_acq_rel);
    check(swapped && counter.load(std::memory_order_acquire) == 5000, "std::atomic fetch_add and CAS");

    // 7. devicetree: present on Arm and RISC-V boards and QEMU virt; absent on the PC
    if (env.dtb == nullptr) {
        kprintf("  skip devicetree tests: this platform describes itself another way (ACPI)\n");
    } else {
        fdt::Blob dt;
        fdt::Node n;
        uint64_t base = 0, size = 0;
        check(dt.init(env.dtb), "devicetree header valid");
        check(dt.find_path("/memory", &n) && dt.reg(n, 0, &base, &size) && size != 0,
              "devicetree memory node has a non-empty range");
        kprintf("  info memory 0x%lx, %lu MiB\n", static_cast<unsigned long>(base),
                static_cast<unsigned long>(size >> 20));
        check(dt.find_path("/chosen", &n), "devicetree has /chosen");
    }
    kprintf("neutral tests on %s: %d passed, %d failed\n", env.arch_name, g_pass, g_fail);
    return g_fail;
}
