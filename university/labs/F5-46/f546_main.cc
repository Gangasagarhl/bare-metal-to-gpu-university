// f546_main.cc - DS403 cluster kernel, F5-46 lab: node 1 serves files, nodes 2 and 3 share
// them through DFS. Command line: node=<id> nodes=<n> life=<ticks> role=server|writer|appender
// server=<node> cache=valid|naive
#include "dfs.h"
#include "k.h"
#include "member.h"
#include "msg.h"

namespace {
char buf[512];

void show(const char* what, int n)
{
    if (n < 0) { klog("%s: error %d (%s)", what, n, dfs_error(static_cast<int8_t>(n))); return; }
    buf[n] = '\0';
    klog("%s: %u bytes", what, n);
    kprintf("  | ");
    for (int i = 0; i < n; ++i) kprintf(buf[i] == '\n' ? (i + 1 < n ? "\n  | " : "\n") : "%c", buf[i]);
    if (n == 0 || buf[n - 1] != '\n') kprintf("\n");
}

void writer_part1()
{
    klog("attach: %s", dfs_error(dfs_attach(0)));
    show("read / (the directory)", dfs_read(0, nullptr, 0, buf, sizeof buf - 1));
    klog("walk motd: %s", dfs_error(dfs_walk(0, 1, "motd", nullptr)));
    show("read motd", dfs_read(1, "motd", 0, buf, sizeof buf - 1));
    klog("create notes: %s", dfs_error(dfs_create(0, 2, "notes")));
    const char line[] = "hello from node 2\n";
    klog("write notes: %d bytes", dfs_write(2, 0, line, sizeof line - 1));
    show("read notes", dfs_read(2, "notes", 0, buf, sizeof buf - 1));
}

void appender()
{
    klog("attach: %s", dfs_error(dfs_attach(0)));
    DfsR q{};
    int8_t e = dfs_walk(0, 1, "notes", &q);
    klog("walk notes: %s (qpath %u version %u length %u)", dfs_error(e), q.qpath, q.qversion, q.length);
    show("read notes", dfs_read(1, "notes", 0, buf, sizeof buf - 1));
    const char line[] = "and hello from node 3\n";
    klog("append notes at offset %u: %d bytes", q.length, dfs_write(1, q.length, line, sizeof line - 1));
    DfsR s{};
    dfs_stat(1, &s);
    klog("stat notes: version %u length %u", s.qversion, s.length);
    klog("create notes again: %s", dfs_error(dfs_create(0, 2, "notes")));
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t mbinfo)
{
    serial_init();
    cmdline_init(magic, mbinfo);
    g_node = static_cast<uint8_t>(arg_u32("node", 1));
    clock_init();
    klog("DS403 cluster kernel (F5-46), node %u, loaded by \"%s\"", g_node, boot_loader_name());
    if (!net_init()) { klog("no e1000 found or link down"); qemu_exit(0x11); }
    member_init(static_cast<uint8_t>(arg_u32("nodes", 3)), 10, 50);
    const uint32_t life = arg_u32("life", 1500);
    const bool srv = arg_is("role", "server");
    if (srv) dfs_server_init();
    else dfs_client_init(static_cast<uint8_t>(arg_u32("server", 1)), arg_is("cache", "naive"));
    int step = 0;
    Msg m;
    for (;;) {
        while (net_poll(m)) {
            member_heard(m);
            if (srv) dfs_server_handle(m);
        }
        member_tick();
        uint32_t t = now();
        if (arg_is("role", "writer")) {
            if (step == 0 && t >= 200) { step = 1; writer_part1(); }
            if (step == 1 && t >= 600) { step = 2; show("read notes again", dfs_read(2, "notes", 0, buf, sizeof buf - 1)); }
            if (step == 2 && t >= 1000) { step = 3; show("read notes after the server left", dfs_read(2, "notes", 0, buf, sizeof buf - 1)); }
        }
        if (arg_is("role", "appender") && step == 0 && t >= 400) { step = 1; appender(); }
        if (srv && step == 0 && t + 20 >= life) { step = 1; dfs_server_dump(); }
        if (t >= life) {
            klog("life of %u ticks reached, halting", life);
            qemu_exit(0x10);
        }
    }
}
