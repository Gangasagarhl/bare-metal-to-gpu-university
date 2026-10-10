// mp1_core.cc - MP1 starter: the arch-neutral regression kernel core (see mp1_core.h).
// It reuses, unchanged: F3-18's kprintf (kprint.h) and F4-23's arch-neutral test suite
// (neutral_tests.h). New here: the stage timeline, the CPU-count test, the root-device
// stage, and the one-line-per-result protocol the harness reads.
#include "mp1_core.h"
#include <cstdint>
#include "bootargs.h"
#include "kprint.h"
#include "neutral_tests.h"

namespace {

int g_pass = 0, g_fail = 0, g_skip = 0;

void result(bool ok, const char* name, const char* detail)
{
    kprintf("MP1 TEST %s %s %s\n", name, ok ? "PASS" : "FAIL", detail);
    ok ? ++g_pass : ++g_fail;
}

void skip(const char* name, const char* reason)
{
    kprintf("MP1 SKIP %s %s\n", name, reason);
    ++g_skip;
}

// One timeline entry. The counter is printed raw; microseconds only when the port knows
// the counter's frequency (AArch64 reads it from CNTFRQ_EL0; x86-64 leaves calibration to
// B8, so it prints ticks only). The harness also stamps every line with the host clock.
void stage(const mp1::Platform& p, const char* name)
{
    uint64_t t = p.now();
    if (p.tick_hz != 0) {
        uint64_t per_us = p.tick_hz / 1000000 == 0 ? 1 : p.tick_hz / 1000000;
        uint64_t us = (t - p.entry_ticks) / per_us;
        kprintf("MP1 STAGE %s ticks=%lu us=%lu\n", name, static_cast<unsigned long>(t),
                static_cast<unsigned long>(us));
    } else {
        kprintf("MP1 STAGE %s ticks=%lu\n", name, static_cast<unsigned long>(t));
    }
}

} // namespace

namespace mp1 {

int run(const Platform& p)
{
    g_pass = g_fail = g_skip = 0;
    kprintf("MP1 BEGIN arch=%s boot=%s\n", p.arch, p.boot);
    kprintf("bootargs: \"%s\"\n", p.bootargs);
    stage(p, "entry-to-core");

    // 1. The arch-neutral suite of DR402 (F4-23), unchanged: its own lines, then one verdict.
    int neutral_failures = run_neutral_tests(NeutralEnv{p.arch, p.dtb, p.page_size});
    result(neutral_failures == 0, "neutral",
           neutral_failures == 0 ? "F4-23 suite clean" : "F4-23 suite failed");
    stage(p, "neutral-tests");

    // 2. CPUs: what the firmware tables list must equal what the harness started (-smp N,
    //    passed as cpus=N) and must fit in what this kernel can track.
    uint64_t want = 0;
    int listed = p.count_cpus();
    if (!bootargs::get_u64(p.bootargs, "cpus", &want)) {
        skip("cpus", "no cpus=N on the command line");
    } else if (listed < 0) {
        result(false, "cpus", "the firmware tables could not be read");
    } else {
        bool ok = static_cast<uint64_t>(listed) == want && listed <= kMaxCpus;
        kprintf("cpus: firmware lists %d, harness started %lu, kernel tracks at most %d\n", listed,
                static_cast<unsigned long>(want), kMaxCpus);
        const char* why = listed > kMaxCpus ? "more CPUs than the kernel tracks" : "count differs";
        result(ok, "cpus", ok ? "count matches" : why);
    }
    stage(p, "cpus");

    // 3. Root device. rootwait=MS (default 0) is how long the port may wait for one to
    //    appear; storage_required=1 makes a missing device a failure instead of a skip.
    uint64_t wait_ms = 0;
    uint64_t required = 0;
    bootargs::get_u64(p.bootargs, "rootwait", &wait_ms);
    bootargs::get_u64(p.bootargs, "storage_required", &required);
    if (p.find_root_device == nullptr) {
        skip("root-device", "this port has no storage driver in the starter kernel");
    } else {
        char desc[96] = "";
        bool found = p.find_root_device(wait_ms, desc, sizeof desc);
        if (found) {
            result(true, "root-device", desc);
        } else if (required != 0) {
            result(false, "root-device", desc);
        } else {
            skip("root-device", desc);
        }
    }
    stage(p, "root-device");

    kprintf("MP1 END pass=%d fail=%d skip=%d\n", g_pass, g_fail, g_skip);
    return g_fail;
}

} // namespace mp1
