// dfs_server.cc - DS403 cluster kernel: the DFS file server (one flat directory in RAM).
#include "dfs.h"
#include "k.h"

namespace {
struct File { bool used; char name[28]; uint32_t qpath, version, len; uint8_t data[1024]; };
struct Fid { bool used; uint8_t client; uint32_t fid; int file; };   // file -1 = the root
File files[16];
Fid fids[64];
uint32_t next_qpath = 1;

Fid* find_fid(uint8_t client, uint32_t fid)
{
    for (Fid& f : fids)
        if (f.used && f.client == client && f.fid == fid) return &f;
    return nullptr;
}

Fid* new_fid(uint8_t client, uint32_t fid, int file)
{
    if (Fid* old = find_fid(client, fid)) { old->file = file; return old; }
    for (Fid& f : fids)
        if (!f.used) { f = Fid{true, client, fid, file}; return &f; }
    return nullptr;
}

int find_file(const char* name)
{
    for (int i = 0; i < 16; ++i)
        if (files[i].used && kstrcmp(files[i].name, name) == 0) return i;
    return -1;
}

void fill_qid(DfsR& r, int file)
{
    if (file < 0) { r.qpath = 0; r.qversion = 0; r.length = 0; return; }
    r.qpath = files[file].qpath;
    r.qversion = files[file].version;
    r.length = files[file].len;
}

int add_file(const char* name, const char* text)
{
    for (int i = 0; i < 16; ++i) {
        if (files[i].used) continue;
        File& f = files[i];
        f.used = true;
        kstrlcpy(f.name, name, sizeof f.name);
        f.qpath = next_qpath++;
        f.version = 0;
        f.len = static_cast<uint32_t>(kstrlen(text));
        memcpy(f.data, text, f.len);
        return i;
    }
    return -1;
}
}  // namespace

void dfs_server_init()
{
    add_file("motd", "welcome to the DS403 cluster\n");
    klog("dfs server: serving 1 file from RAM");
}

void dfs_server_handle(const Msg& m)
{
    if (m.type != M_DFS_T || m.len != sizeof(DfsT)) return;
    DfsT t;
    memcpy(&t, m.data, sizeof t);
    DfsR r{};
    r.tag = t.tag;
    r.op = t.op;
    Fid* f = (t.op == D_ATTACH) ? nullptr : find_fid(m.from, t.fid);
    if (t.op != D_ATTACH && f == nullptr) {
        r.err = DE_BADF;
    } else {
        switch (t.op) {
        case D_ATTACH:
            if (new_fid(m.from, t.fid, -1) == nullptr) r.err = DE_NOSPC;
            break;
        case D_WALK: {
            int i = find_file(t.name);
            if (f->file != -1 || i < 0) { r.err = DE_NOENT; break; }
            if (new_fid(m.from, t.newfid, i) == nullptr) { r.err = DE_NOSPC; break; }
            fill_qid(r, i);
            break;
        }
        case D_CREATE: {
            if (f->file != -1) { r.err = DE_NOENT; break; }
            if (find_file(t.name) >= 0) { r.err = DE_EXIST; break; }
            int i = add_file(t.name, "");
            if (i < 0 || new_fid(m.from, t.newfid, i) == nullptr) { r.err = DE_NOSPC; break; }
            fill_qid(r, i);
            klog("dfs server: node %u created '%s' (qpath %u)", m.from, t.name, files[i].qpath);
            break;
        }
        case D_OPEN:
        case D_STAT:
            fill_qid(r, f->file);
            break;
        case D_READ:
            if (f->file < 0) {   // reading the root lists the names, one per line
                uint16_t n = 0;
                for (const File& x : files) {
                    if (!x.used) continue;
                    size_t l = kstrlen(x.name);
                    if (n + l + 1 > sizeof r.data) break;
                    memcpy(r.data + n, x.name, l);
                    n = static_cast<uint16_t>(n + l);
                    r.data[n++] = '\n';
                }
                r.count = n;
            } else {
                File& x = files[f->file];
                uint32_t off = t.offset < x.len ? t.offset : x.len;
                uint32_t n = x.len - off;
                if (n > t.count) n = t.count;
                if (n > sizeof r.data) n = sizeof r.data;
                memcpy(r.data, x.data + off, n);
                r.count = static_cast<uint16_t>(n);
                fill_qid(r, f->file);
            }
            break;
        case D_WRITE: {
            if (f->file < 0) { r.err = DE_BADF; break; }
            File& x = files[f->file];
            if (t.count > sizeof t.data || t.offset + t.count > sizeof x.data) { r.err = DE_NOSPC; break; }
            memcpy(x.data + t.offset, t.data, t.count);
            if (t.offset + t.count > x.len) x.len = t.offset + t.count;
            ++x.version;
            r.count = t.count;
            fill_qid(r, f->file);
            klog("dfs server: node %u wrote %u bytes at %u to '%s' -> version %u, length %u",
                 m.from, t.count, t.offset, x.name, x.version, x.len);
            break;
        }
        case D_CLUNK:
            f->used = false;
            break;
        default:
            r.err = DE_IO;
        }
    }
    net_send(m.from, M_DFS_R, 0, &r, sizeof r);
}

void dfs_server_dump()
{
    klog("dfs server: file table");
    for (const File& x : files) {
        if (!x.used) continue;
        kprintf("  %s  qpath=%u version=%u length=%u  \"", x.name, x.qpath, x.version, x.len);
        for (uint32_t i = 0; i < x.len; ++i) kprintf(x.data[i] == '\n' ? "\\n" : "%c", x.data[i]);
        kprintf("\"\n");
    }
}
