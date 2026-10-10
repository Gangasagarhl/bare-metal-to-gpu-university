// f544_main.cc - DS403 cluster kernel, F5-44 lab: three nodes form a membership view and
// offer two single-system-image services (cluster process list, remote run).
// Command line: node=<id> nodes=<count> life=<ticks> role=ctl|member hb=<ticks> suspect=<ticks>
#include "k.h"
#include "member.h"
#include "msg.h"
#include "ssi.h"

extern "C" void kmain(uint32_t magic, uint32_t mbinfo)
{
    serial_init();
    cmdline_init(magic, mbinfo);
    g_node = static_cast<uint8_t>(arg_u32("node", 1));
    clock_init();
    klog("DS403 cluster kernel (F5-44), node %u of %u", g_node, arg_u32("nodes", 3));
    if (!net_init()) { klog("no e1000 found or link down"); qemu_exit(0x11); }
    member_init(static_cast<uint8_t>(arg_u32("nodes", 3)), arg_u32("hb", 10), arg_u32("suspect", 50));
    const uint32_t life = arg_u32("life", 1200);
    const bool ctl = arg_is("role", "ctl");
    bool did_cps1 = false, did_run = false, did_cps2 = false, did_run2 = false;
    Msg m;
    for (;;) {
        while (net_poll(m)) {
            member_heard(m);
            ssi_handle(m);
        }
        member_tick();
        ssi_tick();
        uint32_t t = now();
        if (ctl && !did_cps1 && t >= 300) { did_cps1 = true; ssi_cps_start(); }
        if (ctl && !did_run && t >= 450) {
            did_run = true;
            ssi_rrun(1, 20000);
            ssi_rrun(2, 30000);
        }
        if (ctl && !did_cps2 && t >= 900) { did_cps2 = true; ssi_cps_start(); }
        if (ctl && !did_run2 && t >= 1050) { did_run2 = true; ssi_rrun(3, 10000); }
        if (t >= life) {
            klog("life of %u ticks reached, halting", life);
            qemu_exit(0x10);
        }
    }
}
