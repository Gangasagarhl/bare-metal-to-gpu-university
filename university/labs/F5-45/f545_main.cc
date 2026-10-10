// f545_main.cc - DS403 kernel, F5-45 lab: speak 9P to QEMU's 9P server, the way a Plan 9
// terminal speaks to its file server. Command line: node=<id> walk=<file name to read>
#include "k.h"
#include "p9.h"
#include "virtio9p.h"

namespace {
P9Out out;
uint8_t rep[32768];
uint16_t next_tag = 1;
bool show_hex = true;

// Send the message in `out`, print both directions, return the reply (or type 0).
P9In call()
{
    uint32_t len = out.finish();
    kprintf("-> %s tag %u (%u bytes)\n", p9_name(out.buf[4]), out.buf[5] | (out.buf[6] << 8), len);
    if (show_hex) p9_hex(out.buf, len);
    int got = v9p_call(out.buf, len, rep, sizeof rep);
    P9In in{rep, got > 0 ? static_cast<uint32_t>(got) : 0, 0};
    if (got < 7) { kprintf("<- no reply\n"); rep[4] = 0; return in; }
    uint32_t size = rep[0] | (rep[1] << 8) | (rep[2] << 16) | (static_cast<uint32_t>(rep[3]) << 24);
    kprintf("<- %s tag %u (%u bytes)\n", p9_name(in.type()), in.tag(), size);
    if (show_hex) p9_hex(rep, size < 64 ? size : 64);
    in.start();
    if (in.type() == Rerror) {
        char ename[128];
        in.str(ename, sizeof ename);
        uint32_t err = in.u32();                     // 9P2000.u adds a numeric error
        kprintf("   error \"%s\" (errno %u)\n", ename, err);
    }
    return in;
}

void qid(P9In& in, const char* what)
{
    uint8_t type = in.u8();
    uint32_t version = in.u32();
    uint64_t path = in.u64();
    kprintf("   %s qid: type 0x%02x version %u path %u\n", what, type, version, static_cast<uint32_t>(path));
}

bool version(const char* v)
{
    out.begin(Tversion, NOTAG);
    out.u32(32768);                                  // msize: the largest message we accept
    out.str(v);
    P9In in = call();
    if (in.type() != Rversion) return false;
    uint32_t msize = in.u32();
    char got[32];
    in.str(got, sizeof got);
    kprintf("   server msize %u, version \"%s\"\n", msize, got);
    return kstrcmp(got, v) == 0;
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t mbinfo)
{
    serial_init();
    cmdline_init(magic, mbinfo);
    g_node = static_cast<uint8_t>(arg_u32("node", 1));
    clock_init();
    char tag[64];
    if (!v9p_init(tag, sizeof tag)) { klog("no virtio-9p device"); qemu_exit(0x11); }
    klog("virtio-9p: mount tag \"%s\"", tag);

    // 1. Agree on a protocol version. Classic 9P2000 first, then the Unix extension.
    if (!version("9P2000") && !version("9P2000.u")) { klog("no common version"); qemu_exit(0x12); }
    show_hex = false;

    // 2. Attach: fid 0 becomes the root of the served tree.
    out.begin(Tattach, next_tag++);
    out.u32(0); out.u32(NOFID); out.str("ds403"); out.str(""); out.u32(1000);   // .u: n_uname
    P9In in = call();
    if (in.type() != Rattach) qemu_exit(0x13);
    qid(in, "root");

    // 3. Walk from the root to the file named on the command line; fid 1 = that file.
    char name[32] = "hello.txt";
    if (arg_is("walk", "Hello.txt")) kstrlcpy(name, "Hello.txt", sizeof name);
    show_hex = true;
    out.begin(Twalk, next_tag++);
    out.u32(0); out.u32(1); out.u16(1); out.str(name);
    in = call();
    show_hex = false;
    if (in.type() != Rwalk) { klog("walk to %s failed", name); qemu_exit(0x10); }
    kprintf("   nwqid %u\n", in.u16());
    qid(in, name);

    // 4. Open for reading (mode 0 = OREAD) and read it.
    out.begin(Topen, next_tag++);
    out.u32(1); out.u8(0);
    in = call();
    qid(in, "opened");
    out.begin(Tread, next_tag++);
    out.u32(1); out.u64(0); out.u32(512);
    in = call();
    uint32_t count = in.u32();
    kprintf("   %u bytes: \"", count);
    for (uint32_t i = 0; i < count; ++i) kprintf(rep[11 + i] == '\n' ? "\\n" : "%c", rep[11 + i]);
    kprintf("\"\n");

    // 5. Clone the root (walk with no names) into fid 2, create a file there and write it.
    out.begin(Twalk, next_tag++);
    out.u32(0); out.u32(2); out.u16(0);
    call();
    out.begin(Tcreate, next_tag++);
    out.u32(2); out.str("from-kernel.txt"); out.u32(0644); out.u8(1); out.str("");   // OWRITE; .u extension
    in = call();
    if (in.type() == Rcreate) qid(in, "created");
    const char text[] = "written by the DS403 kernel over 9P\n";
    out.begin(Twrite, next_tag++);
    out.u32(2); out.u64(0); out.u32(sizeof text - 1); out.bytes(text, sizeof text - 1);
    in = call();
    kprintf("   wrote %u bytes\n", in.u32());

    // 6. Clunk (forget) every fid we made.
    for (uint32_t f = 2; f != 0xFFFFFFFFu; f = (f == 2 ? 1 : (f == 1 ? 0 : 0xFFFFFFFFu))) {
        out.begin(Tclunk, next_tag++);
        out.u32(f);
        call();
    }
    klog("9P session finished");
    qemu_exit(0x10);
}
