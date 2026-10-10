// b14_main.cc - F3-31 mini kernel: mount the initial RAM disk and test the VFS (milestone B14).
#include "vfs.h"

namespace {
struct MultibootInfo {                   // Multiboot Specification 0.6.96, section 3.3 (first fields)
    uint32_t flags, mem_lower, mem_upper, boot_device, cmdline, mods_count, mods_addr;
};
struct MultibootModule { uint32_t mod_start, mod_end, string, reserved; };

int failures = 0;
void expect(bool ok, const char* what)
{
    kprintf("%s %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) ++failures;
}

// Print "type mode size path" for every entry below dir, depth first, in readdir order.
void walk(const char* dir)
{
    int fd = vfs_open(dir);
    DirEnt e;
    while (fd >= 0 && vfs_readdir(fd, &e) == 1) {
        char path[256];
        size_t n = kstrlen(dir);
        kstrlcpy(path, dir, sizeof path);
        if (path[n - 1] != '/') path[n++] = '/';
        kstrlcpy(path + n, e.name, sizeof path - n);
        Stat st;
        vfs_stat(path, &st);
        kprintf("%c %04o %6u %s\n", st.type == VType::Dir ? 'd' : '-', st.mode, st.size, path);
        if (st.type == VType::Dir) walk(path);
    }
    vfs_close(fd);
}

uint32_t fnv1a(int fd)                   // FNV-1a 32-bit hash of a whole file, read in odd chunks
{
    uint8_t buf[37];
    uint32_t h = 2166136261u;
    long got;
    while ((got = vfs_read(fd, buf, sizeof buf)) > 0)
        for (long i = 0; i < got; ++i) h = (h ^ buf[i]) * 16777619u;
    return h;
}
} // namespace

extern "C" void kmain(uint32_t magic, const MultibootInfo* mbi)
{
    serial_init();
    kprintf("F3-31 kernel: magic=0x%x mods=%u\n", magic, mbi->mods_count);
    if (magic != 0x2BADB002 || !(mbi->flags & (1u << 3)) || mbi->mods_count < 1) {
        kputs("PANIC: no initial RAM disk module\n");
        qemu_exit(0x11);
    }
    const auto* mod = reinterpret_cast<const MultibootModule*>(mbi->mods_addr);
    const auto* image = reinterpret_cast<const uint8_t*>(mod->mod_start);
    uint32_t len = mod->mod_end - mod->mod_start;
    kprintf("initrd: %u bytes at 0x%x\n", len, mod->mod_start);

    Vnode* root = tarfs_create(image, len, 0);
    uint32_t files, dirs, skipped, bad;
    tarfs_stats(&files, &dirs, &skipped, &bad);
    kprintf("tarfs: %u files, %u directories, %u skipped, %u bad headers\n", files, dirs, skipped, bad);
    expect(vfs_mount("/", root, "tarfs") == 0, "mount tarfs on /");
    expect(vfs_mount("/sys", infofs_create(1), "infofs") == 0, "mount infofs on /sys");

    kputs("== listing ==\n");
    walk("/");
    kputs("== end of listing ==\n");

    kputs("== /etc/motd, read 7 bytes at a time ==\n");
    int fd = vfs_open("/etc/motd");
    char buf[8];
    long got;
    while ((got = vfs_read(fd, buf, 7)) > 0) { buf[got] = 0; kputs(buf); }
    vfs_close(fd);
    kputs("== end ==\n");

    fd = vfs_open("/docs/numbers.txt");
    kprintf("fnv1a /docs/numbers.txt = %08x\n", fnv1a(fd));
    vfs_close(fd);

    kputs("== VFS tests ==\n");
    expect(vfs_open("/etc/missing") == -E_NOENT, "missing file gives -ENOENT");
    expect(vfs_open("/etc/motd/x") == -E_NOTDIR, "file used as directory gives -ENOTDIR");
    fd = vfs_open("/etc");
    expect(vfs_read(fd, buf, 4) == -E_ISDIR, "read on a directory gives -EISDIR");
    vfs_close(fd);
    expect(vfs_read(fd, buf, 4) == -E_BADF, "read on a closed descriptor gives -EBADF");

    fd = vfs_open("/sys/../etc/hostname");
    got = vfs_read(fd, buf, 7);
    buf[got > 0 ? got : 0] = 0;
    expect(got > 0, "\"..\" leaves the infofs mount and returns to tarfs");
    kprintf("     /sys/../etc/hostname starts with \"%s\"\n", buf);
    vfs_close(fd);

    int a = vfs_open("/etc/motd");
    int b = vfs_dup(a);
    char x[6] = {}, y[6] = {};
    vfs_read(a, x, 5);
    vfs_read(b, y, 5);
    kprintf("     via fd %d: \"%s\", then via its dup fd %d: \"%s\"\n", a, x, b, y);
    expect(kstrncmp(x, y, 5) != 0, "dup shares one offset (second read continues)");
    vfs_close(a);
    vfs_close(b);

    fd = vfs_open("/etc/motd");
    Stat st;
    vfs_stat("/etc/motd", &st);
    vfs_seek(fd, st.size);
    expect(vfs_read(fd, buf, 4) == 0, "read at end of file returns 0");
    vfs_close(fd);

    int opened = 0, last = 0;
    while ((last = vfs_open("/etc/hostname")) >= 0) ++opened;
    kprintf("     opened %d descriptors, then %d\n", opened, last);
    expect(last == -E_MFILE, "descriptor table full gives -EMFILE");
    for (int i = 0; i < opened; ++i) vfs_close(i);

    fd = vfs_open("/sys/mounts");
    kputs("== /sys/mounts ==\n");
    while ((got = vfs_read(fd, buf, 7)) > 0) { buf[got] = 0; kputs(buf); }
    vfs_close(fd);

    kprintf("B14 VFS tests: %d failures\n", failures);
    qemu_exit(failures == 0 ? 0x10 : 0x11);
}
