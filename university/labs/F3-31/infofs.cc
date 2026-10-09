// infofs.cc - F3-31 mini kernel: a second, synthetic file system (mounted on /sys).
// Its files have no storage: their text is generated at each read.
#include "vfs.h"

namespace {
extern const VnodeOps info_ops;
Vnode root, f_version, f_mounts;
char text[512];

uint32_t render(Vnode* vn)              // build the file's text; return its length
{
    uint32_t n = 0;
    auto add = [&](const char* s) { while (*s && n + 1 < sizeof text) text[n++] = *s++; };
    if (vn == &f_version) {
        add("F3-31 mini kernel (32-bit, Multiboot, no paging)\n");
    } else {
        for (int i = 0; i < vfs_mount_count(); ++i) {
            const char* fs = nullptr;
            const char* path = vfs_mount_info(i, &fs);
            add(fs); add(" on "); add(path); add("\n");
        }
    }
    text[n] = 0;
    return n;
}

int info_lookup(Vnode*, const char* name, Vnode** out)
{
    if (kstrcmp(name, "version") == 0) { *out = &f_version; return 0; }
    if (kstrcmp(name, "mounts") == 0) { f_mounts.size = render(&f_mounts); *out = &f_mounts; return 0; }
    return -E_NOENT;
}

long info_read(Vnode* vn, uint32_t off, void* buf, uint32_t n)
{
    uint32_t len = render(vn);
    if (off >= len) return 0;
    if (n > len - off) n = len - off;
    memcpy(buf, text + off, n);
    return n;
}

int info_readdir(Vnode*, uint32_t index, DirEnt* out)
{
    if (index > 1) return 0;
    kstrlcpy(out->name, index == 0 ? "version" : "mounts", sizeof out->name);
    out->type = VType::File;
    return 1;
}

const VnodeOps info_ops = {info_lookup, info_read, info_readdir};
} // namespace

Vnode* infofs_create(uint32_t dev)
{
    root = Vnode{&info_ops, VType::Dir, 0555, 0, 1, dev, nullptr, nullptr};
    f_version = Vnode{&info_ops, VType::File, 0444, 0, 2, dev, nullptr, nullptr};
    f_version.size = render(&f_version);
    f_mounts = Vnode{&info_ops, VType::File, 0444, 0, 3, dev, nullptr, nullptr};
    return &root;
}
