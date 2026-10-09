// fdt.h - DR403: a small read-only reader for a flattened devicetree blob (DTB).
//
// One header, used unchanged in three places: host tools (fdtdump.cc, the SoC model of F4-35),
// the bare-metal AArch64 kernel of F4-31 to F4-37, and the UEFI application of F4-33.
// Freestanding: no heap, no exceptions, no library calls, only byte loads (the kernel runs with
// the MMU off, where unaligned loads fault, so every multi-byte value is assembled from bytes).
//
// The layout (header fields, the five tokens, 4-byte alignment, big-endian cells, the defaults
// of #address-cells = 2 and #size-cells = 1, the meaning of "ranges") was written after the
// Devicetree Specification, chapters "Flattened Devicetree (DTB) Format" and "Standard
// Properties" (title only - not opened during this build; check before reuse). The labs show
// that it reads the DTB that QEMU 8.2.2 generates for its virt machine, and the F4-31 lab
// cross-checks the addresses it decodes against QEMU's own memory-tree listing.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace fdt {

inline uint32_t be32(const uint8_t* p)
{
    return (uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | uint32_t{p[3]};
}

inline bool streq(const char* a, const char* b)
{
    while (*a != '\0' && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

inline uint32_t slen(const char* s)
{
    uint32_t n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

constexpr uint32_t kMagic = 0xd00dfeed;
constexpr uint32_t kBeginNode = 1;
constexpr uint32_t kEndNode = 2;
constexpr uint32_t kProp = 3;
constexpr uint32_t kNop = 4;
constexpr uint32_t kEnd = 9;
constexpr int kNone = -1;       // "no node"
constexpr int kMaxDepth = 16;

struct Prop {
    const char* name = nullptr;
    const uint8_t* data = nullptr;
    uint32_t len = 0;
    uint32_t u32(uint32_t index = 0) const { return be32(data + 4 * index); }
    uint32_t cells() const { return len / 4; }
};

class Tree {
public:
    // Accepts a blob only if the header is sane; err names the first problem found.
    bool init(const void* blob, const char** err = nullptr)
    {
        const char* e = nullptr;
        base_ = static_cast<const uint8_t*>(blob);
        if (base_ == nullptr) {
            e = "null pointer";
        } else if (be32(base_) != kMagic) {
            e = "bad magic (not a DTB)";
        } else {
            total_ = be32(base_ + 4);
            off_struct_ = be32(base_ + 8);
            off_strings_ = be32(base_ + 12);
            off_rsvmap_ = be32(base_ + 16);
            version_ = be32(base_ + 20);
            size_strings_ = be32(base_ + 32);
            size_struct_ = be32(base_ + 36);
            if (version_ < 17) {
                e = "version older than 17";
            } else if (off_struct_ + size_struct_ > total_ || off_strings_ + size_strings_ > total_) {
                e = "block outside totalsize";
            }
        }
        if (err != nullptr) {
            *err = e;
        }
        ok_ = (e == nullptr);
        return ok_;
    }

    uint32_t total_size() const { return total_; }
    uint32_t version() const { return version_; }
    uint32_t struct_size() const { return size_struct_; }
    uint32_t strings_size() const { return size_strings_; }
    uint32_t boot_cpu() const { return be32(base_ + 28); }
    int root() const { return ok_ ? skip_nops(0) : kNone; }

    // Memory reservation block: pairs of 64-bit (address, size), ended by (0, 0).
    bool reserved(int index, uint64_t& addr, uint64_t& size) const
    {
        const uint8_t* p = base_ + off_rsvmap_ + 16 * static_cast<uint32_t>(index);
        addr = (uint64_t{be32(p)} << 32) | be32(p + 4);
        size = (uint64_t{be32(p + 8)} << 32) | be32(p + 12);
        return addr != 0 || size != 0;
    }

    const char* name(int node) const { return reinterpret_cast<const char*>(at(node + 4)); }

    // Properties of one node, in blob order: index 0, 1, 2 ... until it returns false.
    bool prop_at(int node, int index, Prop& out) const
    {
        uint32_t off = after_name(node);
        for (int i = 0;; ++i) {
            off = skip_nops(off);
            if (tok(off) != kProp) {
                return false;
            }
            if (i == index) {
                out.len = be32(at(off + 4));
                out.name = reinterpret_cast<const char*>(base_ + off_strings_ + be32(at(off + 8)));
                out.data = at(off + 12);
                return true;
            }
            off = align4(off + 12 + be32(at(off + 4)));
        }
    }

    bool prop(int node, const char* pname, Prop& out) const
    {
        for (int i = 0; prop_at(node, i, out); ++i) {
            if (streq(out.name, pname)) {
                return true;
            }
        }
        return false;
    }

    uint32_t u32(int node, const char* pname, uint32_t fallback) const
    {
        Prop p;
        return (prop(node, pname, p) && p.len >= 4) ? p.u32() : fallback;
    }

    int first_child(int node) const
    {
        uint32_t off = after_name(node);
        while (true) {
            off = skip_nops(off);
            if (tok(off) == kProp) {
                off = align4(off + 12 + be32(at(off + 4)));
            } else {
                return tok(off) == kBeginNode ? static_cast<int>(off) : kNone;
            }
        }
    }

    int next_sibling(int node) const
    {
        uint32_t off = skip_nops(end_of(node));
        return tok(off) == kBeginNode ? static_cast<int>(off) : kNone;
    }

    // Document order: the next node after this one, or kNone at the end.
    int next_node(int node, int* depth = nullptr) const
    {
        int c = first_child(node);
        if (c != kNone) {
            if (depth != nullptr) {
                ++*depth;
            }
            return c;
        }
        int n = node;
        while (n != kNone) {
            int s = next_sibling(n);
            if (s != kNone) {
                return s;
            }
            n = parent(n);
            if (depth != nullptr) {
                --*depth;
            }
        }
        return kNone;
    }

    int parent(int node) const
    {
        int stack[kMaxDepth + 1];
        int depth = 0;
        uint32_t off = static_cast<uint32_t>(root());
        while (off < size_struct_) {
            uint32_t t = tok(off);
            if (t == kBeginNode) {
                if (static_cast<int>(off) == node) {
                    return depth > 0 ? stack[depth - 1] : kNone;
                }
                if (depth > kMaxDepth) {
                    return kNone;
                }
                stack[depth++] = static_cast<int>(off);
                off = after_name(static_cast<int>(off));
            } else if (t == kEndNode) {
                --depth;
                off += 4;
            } else if (t == kProp) {
                off = align4(off + 12 + be32(at(off + 4)));
            } else if (t == kNop) {
                off += 4;
            } else {
                return kNone;
            }
        }
        return kNone;
    }

    int find_phandle(uint32_t ph) const
    {
        for (int n = root(); n != kNone; n = next_node(n)) {
            if (u32(n, "phandle", 0) == ph) {
                return n;
            }
        }
        return kNone;
    }

    // "/", "/cpus", "/soc/serial@..." etc. Exact names only (with the unit address).
    int find_path(const char* path) const
    {
        int n = root();
        const char* p = path;
        while (n != kNone && *p == '/') {
            ++p;
            if (*p == '\0') {
                return n;
            }
            uint32_t len = 0;
            while (p[len] != '\0' && p[len] != '/') {
                ++len;
            }
            int c = first_child(n);
            while (c != kNone && !same_part(name(c), p, len)) {
                c = next_sibling(c);
            }
            n = c;
            p += len;
        }
        return n;
    }

    // Does the "compatible" string list of this node contain want?
    bool compatible(int node, const char* want) const
    {
        Prop p;
        if (!prop(node, "compatible", p)) {
            return false;
        }
        for (uint32_t i = 0; i < p.len;) {
            const char* s = reinterpret_cast<const char*>(p.data + i);
            if (streq(s, want)) {
                return true;
            }
            i += slen(s) + 1;
        }
        return false;
    }

    // "status" absent, "okay" or "ok" means the node is in use; "disabled" and others mean not.
    bool available(int node) const
    {
        Prop p;
        if (!prop(node, "status", p)) {
            return true;
        }
        const char* s = reinterpret_cast<const char*>(p.data);
        return streq(s, "okay") || streq(s, "ok");
    }

    // #address-cells / #size-cells that apply to the children of bus (defaults 2 and 1).
    uint32_t addr_cells(int bus) const { return u32(bus, "#address-cells", 2); }
    uint32_t size_cells(int bus) const { return u32(bus, "#size-cells", 1); }

    // Entry i of this node's "reg", in the parent bus's address space (not translated).
    bool reg(int node, int i, uint64_t& addr, uint64_t& size) const
    {
        int bus = parent(node);
        Prop p;
        if (bus == kNone || !prop(node, "reg", p)) {
            return false;
        }
        uint32_t ac = addr_cells(bus);
        uint32_t sc = size_cells(bus);
        uint32_t idx = static_cast<uint32_t>(i) * (ac + sc);
        if (ac == 0 || ac > 2 || sc > 2 || (idx + ac + sc) * 4 > p.len) {
            return false;
        }
        addr = read_cells(p, idx, ac);
        size = sc == 0 ? 0 : read_cells(p, idx + ac, sc);
        return true;
    }

    // Translate an address on the bus of `node` (its parent) to a CPU physical address by
    // walking up through every "ranges" property. Absent "ranges" means: not translatable.
    bool translate(int node, uint64_t& addr) const
    {
        int bus = parent(node);
        while (bus != kNone && bus != root()) {
            int up = parent(bus);
            Prop r;
            if (!prop(bus, "ranges", r)) {
                return false;
            }
            if (r.len != 0) {
                uint32_t ca = addr_cells(bus);
                uint32_t pa = addr_cells(up);
                uint32_t cs = size_cells(bus);
                uint32_t step = ca + pa + cs;
                bool hit = false;
                for (uint32_t k = 0; step != 0 && (k + step) * 4 <= r.len; k += step) {
                    uint64_t child = read_cells(r, k, ca);
                    uint64_t par = read_cells(r, k + ca, pa);
                    uint64_t len = read_cells(r, k + ca + pa, cs);
                    if (addr >= child && addr - child < len) {
                        addr = addr - child + par;
                        hit = true;
                        break;
                    }
                }
                if (!hit) {
                    return false;
                }
            }
            bus = up;
        }
        return true;
    }

    // reg entry i, translated to a CPU address.
    bool mmio(int node, int i, uint64_t& addr, uint64_t& size) const
    {
        return reg(node, i, addr, size) && translate(node, addr);
    }

    // Writes "/a/b@1" into buf (cut to cap - 1 characters).
    void path(int node, char* buf, uint32_t cap) const
    {
        int chain[kMaxDepth + 1];
        int n = 0;
        for (int x = node; x != kNone && n <= kMaxDepth; x = parent(x)) {
            chain[n++] = x;
        }
        uint32_t len = 0;
        auto put = [&](char c) {
            if (len + 1 < cap) {
                buf[len++] = c;
            }
        };
        if (n <= 1) {
            put('/');
        }
        for (int k = n - 2; k >= 0; --k) {
            put('/');
            for (const char* s = name(chain[k]); *s != '\0'; ++s) {
                put(*s);
            }
        }
        if (cap > 0) {
            buf[len] = '\0';
        }
    }

    static uint64_t read_cells(const Prop& p, uint32_t first, uint32_t count)
    {
        uint64_t v = 0;
        for (uint32_t c = 0; c < count; ++c) {
            v = (v << 32) | p.u32(first + c);
        }
        return v;
    }

private:
    const uint8_t* at(uint32_t struct_off) const { return base_ + off_struct_ + struct_off; }
    uint32_t tok(uint32_t off) const { return off + 4 <= size_struct_ ? be32(at(off)) : kEnd; }
    static uint32_t align4(uint32_t v) { return (v + 3u) & ~3u; }
    uint32_t skip_nops(uint32_t off) const
    {
        while (tok(off) == kNop) {
            off += 4;
        }
        return off;
    }
    uint32_t skip_nops(int off) const { return skip_nops(static_cast<uint32_t>(off)); }
    uint32_t after_name(int node) const
    {
        const char* n = name(node);
        return align4(static_cast<uint32_t>(node) + 4 + slen(n) + 1);
    }
    // Offset just after the END_NODE that closes this node.
    uint32_t end_of(int node) const
    {
        int depth = 0;
        uint32_t off = static_cast<uint32_t>(node);
        while (off < size_struct_) {
            uint32_t t = tok(off);
            if (t == kBeginNode) {
                ++depth;
                off = after_name(static_cast<int>(off));
            } else if (t == kEndNode) {
                off += 4;
                if (--depth == 0) {
                    return off;
                }
            } else if (t == kProp) {
                off = align4(off + 12 + be32(at(off + 4)));
            } else if (t == kNop) {
                off += 4;
            } else {
                return size_struct_;
            }
        }
        return size_struct_;
    }
    static bool same_part(const char* node_name, const char* p, uint32_t len)
    {
        for (uint32_t i = 0; i < len; ++i) {
            if (node_name[i] != p[i]) {
                return false;
            }
        }
        return node_name[len] == '\0';
    }

    const uint8_t* base_ = nullptr;
    uint32_t total_ = 0;
    uint32_t off_struct_ = 0;
    uint32_t off_strings_ = 0;
    uint32_t off_rsvmap_ = 0;
    uint32_t version_ = 0;
    uint32_t size_strings_ = 0;
    uint32_t size_struct_ = 0;
    bool ok_ = false;
};

}  // namespace fdt
