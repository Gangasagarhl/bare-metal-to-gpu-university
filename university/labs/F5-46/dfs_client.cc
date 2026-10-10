// dfs_client.cc - DS403 cluster kernel: the DFS client (remote procedure calls + a cache).
#include "dfs.h"
#include "k.h"
#include "member.h"

namespace {
uint8_t server = 1;
bool naive = false;
uint16_t next_tag = 1;
constexpr uint32_t WAIT = 100;         // ticks before a retry
constexpr int TRIES = 3;

struct Cached { bool used; char name[28]; uint32_t version; uint16_t len; char data[512]; };
Cached cache[4];

// One remote call: send, then poll until the reply with our tag arrives or time runs out.
int8_t rpc(DfsT& t, DfsR& r)
{
    t.tag = next_tag++;
    for (int attempt = 1; attempt <= TRIES; ++attempt) {
        net_send(server, M_DFS_T, 0, &t, sizeof t);
        uint32_t start = now();
        Msg m;
        while (now() - start < WAIT) {
            member_tick();                                 // keep our heartbeats going
            while (net_poll(m)) {
                member_heard(m);
                if (m.type != M_DFS_R || m.from != server || m.len != sizeof r) continue;
                memcpy(&r, m.data, sizeof r);
                if (r.tag == t.tag) return r.err;
            }
        }
        klog("dfs: no reply to op %u tag %u from node %u (attempt %u of %u)", t.op, t.tag, server, attempt, TRIES);
    }
    return DE_TIMEDOUT;
}

Cached* lookup(const char* name)
{
    for (Cached& c : cache)
        if (c.used && kstrcmp(c.name, name) == 0) return &c;
    return nullptr;
}
}  // namespace

void dfs_client_init(uint8_t srv, bool naive_cache)
{
    server = srv;
    naive = naive_cache;
    klog("dfs client: server is node %u, cache %s", server, naive ? "NAIVE (never revalidates)" : "validated by qid version");
}

const char* dfs_error(int8_t e)
{
    switch (e) {
    case DE_OK: return "ok";
    case DE_NOENT: return "file does not exist";
    case DE_IO: return "i/o error";
    case DE_BADF: return "unknown fid";
    case DE_EXIST: return "file exists";
    case DE_NOSPC: return "no space";
    case DE_TIMEDOUT: return "server unreachable (timed out)";
    default: return "?";
    }
}

int8_t dfs_attach(uint32_t fid)
{
    DfsT t{}; DfsR r{};
    t.op = D_ATTACH; t.fid = fid;
    return rpc(t, r);
}

int8_t dfs_walk(uint32_t fid, uint32_t newfid, const char* name, DfsR* out)
{
    DfsT t{}; DfsR r{};
    t.op = D_WALK; t.fid = fid; t.newfid = newfid;
    kstrlcpy(t.name, name, sizeof t.name);
    int8_t e = rpc(t, r);
    if (out != nullptr) *out = r;
    return e;
}

int8_t dfs_create(uint32_t dirfid, uint32_t newfid, const char* name)
{
    DfsT t{}; DfsR r{};
    t.op = D_CREATE; t.fid = dirfid; t.newfid = newfid;
    kstrlcpy(t.name, name, sizeof t.name);
    return rpc(t, r);
}

int8_t dfs_stat(uint32_t fid, DfsR* out)
{
    DfsT t{};
    t.op = D_STAT; t.fid = fid;
    return rpc(t, *out);
}

int dfs_read(uint32_t fid, const char* name, uint32_t offset, char* buf, uint16_t cap)
{
    Cached* c = (offset == 0 && name != nullptr) ? lookup(name) : nullptr;
    if (c != nullptr && naive) {
        klog("dfs: read '%s' from the cache (version %u, not checked)", name, c->version);
        uint16_t n = c->len < cap ? c->len : cap;
        memcpy(buf, c->data, n);
        return n;
    }
    if (c != nullptr) {                                     // validate: one small round trip
        DfsR s{};
        int8_t e = dfs_stat(fid, &s);
        if (e != DE_OK) return e;
        if (s.qversion == c->version) {
            klog("dfs: read '%s' from the cache (version %u still current)", name, c->version);
            uint16_t n = c->len < cap ? c->len : cap;
            memcpy(buf, c->data, n);
            return n;
        }
        klog("dfs: cached '%s' is version %u, server has %u: fetching", name, c->version, s.qversion);
    }
    DfsT t{}; DfsR r{};
    t.op = D_READ; t.fid = fid; t.offset = offset; t.count = cap;
    int8_t e = rpc(t, r);
    if (e != DE_OK) return e;
    uint16_t n = r.count < cap ? r.count : cap;
    memcpy(buf, r.data, n);
    if (offset == 0 && name != nullptr && r.qpath != 0) {   // remember whole small files
        if (c == nullptr)
            for (Cached& x : cache)
                if (!x.used) { c = &x; break; }
        if (c != nullptr) {
            c->used = true;
            kstrlcpy(c->name, name, sizeof c->name);
            c->version = r.qversion;
            c->len = n;
            memcpy(c->data, r.data, n);
        }
    }
    return n;
}

int dfs_write(uint32_t fid, uint32_t offset, const char* data, uint16_t len)
{
    DfsT t{}; DfsR r{};
    t.op = D_WRITE; t.fid = fid; t.offset = offset; t.count = len;
    memcpy(t.data, data, len);
    int8_t e = rpc(t, r);
    return e != DE_OK ? e : r.count;
}

int8_t dfs_clunk(uint32_t fid)
{
    DfsT t{}; DfsR r{};
    t.op = D_CLUNK; t.fid = fid;
    return rpc(t, r);
}
