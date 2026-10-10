// f547_main.cc - DS403 cluster kernel, F5-47 lab: membership + consensus + scheduler.
// Command line: node=<id> nodes=<n> life=<ticks> mode=raft|naive scen=sched|split
#include "k.h"
#include "member.h"
#include "msg.h"
#include "naive.h"
#include "raft.h"
#include "sm.h"

namespace {
Entry cas(const char* key, const char* expect, const char* val)
{
    Entry e{};
    e.kind = K_CAS;
    kstrlcpy(e.key, key, 8);
    kstrlcpy(e.expect, expect, 8);
    kstrlcpy(e.val, val, 8);
    return e;
}

Entry submit(uint32_t job, uint32_t ticks)
{
    Entry e{};
    e.kind = K_JOB_SUBMIT;
    e.a = job;
    e.b = ticks;
    return e;
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t mbinfo)
{
    serial_init();
    cmdline_init(magic, mbinfo);
    g_node = static_cast<uint8_t>(arg_u32("node", 1));
    clock_init();
    const bool naive = arg_is("mode", "naive");
    klog("DS403 cluster kernel (F5-47), node %u, mode %s", g_node, naive ? "naive" : "raft");
    if (!net_init()) { klog("no e1000 found or link down"); qemu_exit(0x11); }
    const uint8_t nodes = static_cast<uint8_t>(arg_u32("nodes", 3));
    member_init(nodes, 10, 50);
    if (naive) naive_init(); else raft_init(nodes);
    const uint32_t life = arg_u32("life", 1500);
    const bool split = arg_is("scen", "split");
    int step = 0;
    Msg m;
    for (;;) {
        while (net_poll(m)) {
            member_heard(m);
            if (naive) naive_handle(m); else raft_handle(m);
        }
        member_tick();
        if (naive) naive_tick(); else raft_tick();
        uint32_t t = now();
        // Scenario "split": two clients race for the same lock while the network is cut.
        if (split && step == 0 && t >= 900 && (g_node == 1 || g_node == 3)) {
            step = 1;
            if (g_node == 3) {
                Entry e = cas("lock", "", "n3");
                if (naive) naive_client(e); else raft_client(e, 300);
            } else {
                Entry e = cas("lock", "", "n1");
                if (naive) naive_client(e); else raft_client(e, 300);
            }
        }
        // Scenario "sched": node 2 submits four jobs; the leader places them.
        if (!split && step == 0 && t >= 300 && g_node == 2) {
            step = 1;
            for (uint32_t j = 1; j <= 4; ++j) raft_client(submit(j, 400), 300);
        }
        if (step < 9 && t + 5 >= life) { step = 9; sm_dump("final"); }
        if (t >= life) {
            klog("life of %u ticks reached, halting", life);
            qemu_exit(0x10);
        }
    }
}
