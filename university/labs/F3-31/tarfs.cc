// tarfs.cc - F3-31 mini kernel: a read-only file system over a USTAR archive in memory.
// Header layout after POSIX (The Open Group Base Specifications), "pax", ustar format.
#include "vfs.h"

namespace {
struct UstarHeader {                     // one 512-byte block
    char name[100], mode[8], uid[8], gid[8], size[12], mtime[12], chksum[8];
    char typeflag, linkname[100], magic[6], version[2], uname[32], gname[32];
    char devmajor[8], devminor[8], prefix[155], pad[12];
};
static_assert(sizeof(UstarHeader) == 512, "a ustar header is one 512-byte block");

struct TarNode {
    Vnode vn;
    char name[100];
    TarNode* parent;
    TarNode* first_child;
    TarNode* last_child;
    TarNode* next_sibling;
    const uint8_t* data;                 // file contents inside the archive (no copy)
};

constexpr int MAX_NODES = 256;
TarNode nodes[MAX_NODES];
int node_count = 0;
uint32_t n_files, n_dirs, n_skipped, n_bad;
extern const VnodeOps tar_ops;

uint32_t octal(const char* f, size_t n)  // numeric fields are octal ASCII
{
    uint32_t v = 0;
    for (size_t i = 0; i < n && f[i] >= '0' && f[i] <= '7'; ++i) v = v * 8 + static_cast<uint32_t>(f[i] - '0');
    return v;
}

bool checksum_ok(const uint8_t* blk)
{
    uint32_t sum = 0;
    for (int i = 0; i < 512; ++i) sum += (i >= 148 && i < 156) ? ' ' : blk[i];  // chksum field counts as spaces
    return sum == octal(reinterpret_cast<const UstarHeader*>(blk)->chksum, 8);
}

TarNode* new_node(TarNode* parent, const char* name, size_t len, VType type, uint32_t dev)
{
    if (node_count == MAX_NODES) return nullptr;
    TarNode* n = &nodes[node_count];
    memset(n, 0, sizeof *n);
    n->vn = Vnode{&tar_ops, type, type == VType::Dir ? 0755u : 0644u, 0,
                  static_cast<uint32_t>(node_count + 1), dev, n, nullptr};
    memcpy(n->name, name, len);
    n->name[len] = 0;
    n->parent = parent;
    if (parent) {                        // keep children in archive order
        if (parent->last_child) parent->last_child->next_sibling = n; else parent->first_child = n;
        parent->last_child = n;
    }
    ++node_count;
    return n;
}

TarNode* child(TarNode* dir, const char* name, size_t len)
{
    for (TarNode* c = dir->first_child; c; c = c->next_sibling)
        if (kstrlen(c->name) == len && kstrncmp(c->name, name, len) == 0) return c;
    return nullptr;
}

// Walk "a/b/c", creating missing directories; the last component gets the given type.
TarNode* add_path(TarNode* root, const char* path, VType type, uint32_t dev)
{
    TarNode* cur = root;
    const char* p = path;
    while (*p) {
        while (*p == '/') ++p;
        if (*p == 0) break;
        size_t len = 0;
        while (p[len] && p[len] != '/') ++len;
        if (len == 1 && p[0] == '.') { p += len; continue; }
        bool last = true;
        for (const char* q = p + len; *q; ++q) if (*q != '/') { last = false; break; }
        TarNode* c = child(cur, p, len);
        if (!c) c = new_node(cur, p, len, last ? type : VType::Dir, dev);
        if (!c) return nullptr;
        cur = c;
        p += len;
    }
    return cur;
}

int tar_lookup(Vnode* dir, const char* name, Vnode** out)
{
    TarNode* c = child(static_cast<TarNode*>(dir->priv), name, kstrlen(name));
    if (!c) return -E_NOENT;
    *out = &c->vn;
    return 0;
}

long tar_read(Vnode* vn, uint32_t off, void* buf, uint32_t n)
{
    if (off >= vn->size) return 0;
    if (n > vn->size - off) n = vn->size - off;
    memcpy(buf, static_cast<TarNode*>(vn->priv)->data + off, n);
    return n;
}

int tar_readdir(Vnode* dir, uint32_t index, DirEnt* out)
{
    TarNode* c = static_cast<TarNode*>(dir->priv)->first_child;
    while (c && index--) c = c->next_sibling;
    if (!c) return 0;
    kstrlcpy(out->name, c->name, sizeof out->name);
    out->type = c->vn.type;
    return 1;
}

const VnodeOps tar_ops = {tar_lookup, tar_read, tar_readdir};
} // namespace

Vnode* tarfs_create(const uint8_t* image, uint32_t len, uint32_t dev)
{
    TarNode* root = new_node(nullptr, "", 0, VType::Dir, dev);
    for (uint32_t pos = 0; pos + 512 <= len;) {
        const uint8_t* blk = image + pos;
        bool zero = true;
        for (int i = 0; i < 512 && zero; ++i) zero = blk[i] == 0;
        if (zero) break;                 // end of archive: a block of zeros
        const auto* h = reinterpret_cast<const UstarHeader*>(blk);
        uint32_t size = octal(h->size, sizeof h->size);
        if (!checksum_ok(blk) || kstrncmp(h->magic, "ustar", 5) != 0) { ++n_bad; break; }
        char path[257];                  // prefix (155) + "/" + name (100) + NUL
        size_t n = 0;
        for (size_t i = 0; i < sizeof h->prefix && h->prefix[i]; ++i) path[n++] = h->prefix[i];
        if (n) path[n++] = '/';
        for (size_t i = 0; i < sizeof h->name && h->name[i]; ++i) path[n++] = h->name[i];
        path[n] = 0;
        VType type = h->typeflag == '5' ? VType::Dir : VType::File;
        if (h->typeflag == '0' || h->typeflag == 0 || h->typeflag == '5') {
            TarNode* node = add_path(root, path, type, dev);
            if (node) {
                node->vn.mode = octal(h->mode, sizeof h->mode);
                node->vn.size = type == VType::File ? size : 0;
                node->data = blk + 512;
                ++(type == VType::Dir ? n_dirs : n_files);
            }
        } else {
            ++n_skipped;                 // links, devices, FIFOs: not supported here
        }
        pos += 512 + (size + 511) / 512 * 512;   // contents are padded to whole blocks
    }
    return &root->vn;
}

void tarfs_stats(uint32_t* files, uint32_t* dirs, uint32_t* skipped, uint32_t* bad)
{
    *files = n_files; *dirs = n_dirs; *skipped = n_skipped; *bad = n_bad;
}
