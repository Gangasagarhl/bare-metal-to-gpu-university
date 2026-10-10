// vfs.cc - F3-31 mini kernel: mount table, path lookup and file descriptors.
#include "vfs.h"

namespace {
constexpr int MAX_MOUNTS = 8;
constexpr int MAX_OPEN = 32;            // open-file descriptions (shared by dup)
constexpr int MAX_FD = 16;              // descriptors of the one "process" of this kernel
constexpr int MAX_DEPTH = 32;

struct Mount { char path[64]; const char* fs_name; Vnode* root; };
struct OpenFile { Vnode* vn; uint32_t offset; int refs; };

Mount mounts[MAX_MOUNTS];
int mount_count = 0;
Vnode* root_vnode = nullptr;
OpenFile open_files[MAX_OPEN];
OpenFile* fd_table[MAX_FD];

Vnode* cross_mounts(Vnode* vn)
{
    while (vn->mounted) vn = vn->mounted;   // a directory may hide a mounted file system
    return vn;
}
} // namespace

int vfs_lookup(const char* path, Vnode** out)
{
    if (path[0] != '/' || root_vnode == nullptr) return -E_INVAL;
    Vnode* stack[MAX_DEPTH];             // the directories walked so far, for ".."
    int depth = 0;
    stack[depth++] = cross_mounts(root_vnode);
    const char* p = path;
    char name[100];
    while (*p) {
        while (*p == '/') ++p;
        if (*p == 0) break;
        size_t len = 0;
        while (p[len] && p[len] != '/') ++len;
        if (len >= sizeof name) return -E_NAMETOOLONG;
        memcpy(name, p, len);
        name[len] = 0;
        p += len;
        Vnode* cur = stack[depth - 1];
        if (kstrcmp(name, ".") == 0) continue;
        if (kstrcmp(name, "..") == 0) { if (depth > 1) --depth; continue; }
        if (cur->type != VType::Dir) return -E_NOTDIR;
        Vnode* next = nullptr;
        int rc = cur->ops->lookup(cur, name, &next);
        if (rc < 0) return rc;
        if (depth == MAX_DEPTH) return -E_NAMETOOLONG;
        stack[depth++] = cross_mounts(next);
    }
    *out = stack[depth - 1];
    return 0;
}

int vfs_mount(const char* path, Vnode* root, const char* fs_name)
{
    if (mount_count == MAX_MOUNTS) return -E_BUSY;
    if (kstrcmp(path, "/") == 0) {
        if (root_vnode) return -E_BUSY;
        root_vnode = root;
    } else {
        Vnode* covered = nullptr;
        int rc = vfs_lookup(path, &covered);
        if (rc < 0) return rc;
        if (covered->type != VType::Dir) return -E_NOTDIR;
        covered->mounted = root;          // lookups that reach this directory continue in root
    }
    Mount& m = mounts[mount_count++];
    kstrlcpy(m.path, path, sizeof m.path);
    m.fs_name = fs_name;
    m.root = root;
    return 0;
}

int vfs_mount_count() { return mount_count; }
const char* vfs_mount_info(int i, const char** fs_name) { *fs_name = mounts[i].fs_name; return mounts[i].path; }

int vfs_open(const char* path)
{
    Vnode* vn = nullptr;
    int rc = vfs_lookup(path, &vn);
    if (rc < 0) return rc;
    int fd = 0;
    while (fd < MAX_FD && fd_table[fd]) ++fd;
    if (fd == MAX_FD) return -E_MFILE;
    for (OpenFile& of : open_files) {
        if (of.refs == 0) {
            of = OpenFile{vn, 0, 1};
            fd_table[fd] = &of;
            return fd;
        }
    }
    return -E_MFILE;
}

static OpenFile* get(int fd) { return (fd >= 0 && fd < MAX_FD) ? fd_table[fd] : nullptr; }

long vfs_read(int fd, void* buf, uint32_t n)
{
    OpenFile* of = get(fd);
    if (!of) return -E_BADF;
    if (of->vn->type == VType::Dir) return -E_ISDIR;
    long got = of->vn->ops->read(of->vn, of->offset, buf, n);
    if (got > 0) of->offset += static_cast<uint32_t>(got);
    return got;
}

long vfs_seek(int fd, long off)
{
    OpenFile* of = get(fd);
    if (!of) return -E_BADF;
    if (off < 0) return -E_INVAL;
    of->offset = static_cast<uint32_t>(off);
    return off;
}

int vfs_readdir(int fd, DirEnt* out)
{
    OpenFile* of = get(fd);
    if (!of) return -E_BADF;
    if (of->vn->type != VType::Dir) return -E_NOTDIR;
    int rc = of->vn->ops->readdir(of->vn, of->offset, out);
    if (rc == 1) ++of->offset;          // for a directory the offset counts entries
    return rc;
}

int vfs_stat(const char* path, Stat* st)
{
    Vnode* vn = nullptr;
    int rc = vfs_lookup(path, &vn);
    if (rc < 0) return rc;
    *st = Stat{vn->type, vn->mode, vn->size, vn->ino, vn->dev};
    return 0;
}

int vfs_dup(int fd)
{
    OpenFile* of = get(fd);
    if (!of) return -E_BADF;
    for (int nfd = 0; nfd < MAX_FD; ++nfd) {
        if (!fd_table[nfd]) { fd_table[nfd] = of; ++of->refs; return nfd; }
    }
    return -E_MFILE;
}

int vfs_close(int fd)
{
    OpenFile* of = get(fd);
    if (!of) return -E_BADF;
    fd_table[fd] = nullptr;
    --of->refs;                          // the description lives on while another fd uses it
    return 0;
}
