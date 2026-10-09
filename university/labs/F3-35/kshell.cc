// kshell.cc - F3-35: a serial-console shell inside the F3-31 mini kernel. It runs in kernel mode
// (the mini kernel has no user mode); it exists so that a scripted serial session can be checked
// against a reference transcript, the form of OS304's practical exam.
#include "vfs.h"

namespace {
struct MultibootInfo { uint32_t flags, mem_lower, mem_upper, boot_device, cmdline, mods_count, mods_addr; };
struct MultibootModule { uint32_t mod_start, mod_end, string, reserved; };

constexpr int MAX_ARGS = 8;

int split(char* line, char* argv[])                 // cut the line into words at blanks
{
    int argc = 0;
    for (char* p = line; *p && argc < MAX_ARGS;) {
        while (*p == ' ') *p++ = 0;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') ++p;
    }
    return argc;
}

void cmd_ls(const char* dir)
{
    int fd = vfs_open(dir);
    if (fd < 0) { kprintf("ls: %s: error %d\n", dir, fd); return; }
    DirEnt e;
    int rc;
    while ((rc = vfs_readdir(fd, &e)) == 1) {
        char path[256];
        size_t n = kstrlen(dir);
        kstrlcpy(path, dir, sizeof path);
        if (path[n - 1] != '/') path[n++] = '/';
        kstrlcpy(path + n, e.name, sizeof path - n);
        Stat st;
        vfs_stat(path, &st);
        kprintf("%c %04o %6u %s\n", st.type == VType::Dir ? 'd' : '-', st.mode, st.size, e.name);
    }
    if (rc < 0) kprintf("ls: %s: error %d\n", dir, rc);
    vfs_close(fd);
}

void cmd_cat(const char* path)
{
    int fd = vfs_open(path);
    if (fd < 0) { kprintf("cat: %s: error %d\n", path, fd); return; }
    char buf[65];
    long got;
    while ((got = vfs_read(fd, buf, 64)) > 0) { buf[got] = 0; kputs(buf); }
    if (got < 0) kprintf("cat: %s: error %d\n", path, static_cast<int>(got));
    vfs_close(fd);
}

void cmd_stat(const char* path)
{
    Stat st;
    int rc = vfs_stat(path, &st);
    if (rc < 0) { kprintf("stat: %s: error %d\n", path, rc); return; }
    kprintf("%s: %s, mode %04o, size %u, inode %u, file system %u\n", path,
            st.type == VType::Dir ? "directory" : "file", st.mode, st.size, st.ino, st.dev);
}

void execute(char* line)
{
    char* argv[MAX_ARGS];
    int argc = split(line, argv);
    if (argc == 0) return;
    const char* c = argv[0];
    if (kstrcmp(c, "help") == 0) kputs("commands: help echo ls cat stat mounts exit\n");
    else if (kstrcmp(c, "echo") == 0) {
        for (int i = 1; i < argc; ++i) kprintf(i > 1 ? " %s" : "%s", argv[i]);
        kputs("\n");
    } else if (kstrcmp(c, "ls") == 0) cmd_ls(argc > 1 ? argv[1] : "/");
    else if (kstrcmp(c, "cat") == 0 && argc > 1) cmd_cat(argv[1]);
    else if (kstrcmp(c, "stat") == 0 && argc > 1) cmd_stat(argv[1]);
    else if (kstrcmp(c, "mounts") == 0) cmd_cat("/sys/mounts");
    else if (kstrcmp(c, "exit") == 0) { kputs("bye\n"); qemu_exit(0x10); }
    else kprintf("%s: unknown command\n", c);
}
} // namespace

extern "C" void kmain(uint32_t magic, const MultibootInfo* mbi)
{
    serial_init();
    if (magic != 0x2BADB002 || mbi->mods_count < 1) { kputs("PANIC: no initial RAM disk\n"); qemu_exit(0x11); }
    const auto* mod = reinterpret_cast<const MultibootModule*>(mbi->mods_addr);
    vfs_mount("/", tarfs_create(reinterpret_cast<const uint8_t*>(mod->mod_start), mod->mod_end - mod->mod_start, 0), "tarfs");
    vfs_mount("/sys", infofs_create(1), "infofs");
    kputs("OS304 kernel shell. Type help.\n");
    char line[128];
    for (;;) {
        kputs("os304> ");
        size_t n = 0;
        for (;;) {
            char ch = static_cast<char>(serial_getc());
            if (ch == '\r' || ch == '\n') break;
            if ((ch == 0x7F || ch == 0x08) && n > 0) { --n; kputs("\b \b"); continue; }   // backspace
            if (ch < ' ' || n + 1 == sizeof line) continue;     // ignore other control characters
            line[n++] = ch;
            serial_putc(ch);                                     // echo, as a terminal expects
        }
        line[n] = 0;
        serial_putc('\n');
        execute(line);
    }
}
