// fdt.h - F4-23: a small read-only parser for a flattened devicetree blob (DTB).
// Arch-neutral and freestanding: no heap, no library calls, no hardware access.
// The same file is compiled into the host tool (fdt_tool.cc), the host test
// (fdt_test.cpp), the AArch64 kernels (F4-24 ...) and the RISC-V kernels (F4-28 ...).
// Layout (header fields, tokens, alignment) after the Devicetree Specification,
// chapter "Flattened Devicetree (DTB) Format" (title only, pending verification).
// Every multi-byte field in a DTB is big-endian, so every read goes through be32().
#pragma once
#include <cstddef>
#include <cstdint>

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

// "uart@9000000" matches "uart" (the unit address after '@' is ignored) and itself.
inline bool name_matches(const char* node, const char* want)
{
    while (*want != '\0' && *node == *want) {
        ++node;
        ++want;
    }
    return *want == '\0' && (*node == '\0' || *node == '@');
}

constexpr uint32_t kMagic = 0xd00dfeed;
constexpr uint32_t kBeginNode = 1, kEndNode = 2, kProp = 3, kNop = 4, kEnd = 9;
constexpr int kMaxDepth = 16;

struct Prop {
    const char* name = nullptr;
    const uint8_t* data = nullptr;
    uint32_t len = 0;
};

struct Node {
    uint32_t offset = 0;        // offset of the BEGIN_NODE token inside the structure block
    const char* name = "";      // "" for the root node
    int depth = 0;              // root = 0
    uint32_t addr_cells = 2;    // #address-cells of the parent: how to read this node's "reg"
    uint32_t size_cells = 1;    // #size-cells of the parent
};

class Blob {
public:
    // Returns false if p does not start with a DTB header this parser understands.
    bool init(const void* p)
    {
        base_ = static_cast<const uint8_t*>(p);
        if (base_ == nullptr || be32(base_) != kMagic) {
            return false;
        }
        total_ = be32(base_ + 4);
        off_struct_ = be32(base_ + 8);
        off_strings_ = be32(base_ + 12);
        version_ = be32(base_ + 20);
        size_struct_ = be32(base_ + 36);
        return version_ >= 17 && off_struct_ < total_ && off_strings_ < total_ &&
               off_struct_ + size_struct_ <= total_;
    }
    uint32_t total_size() const { return total_; }
    uint32_t version() const { return version_; }

    // Calls fn(node) for every node in document order; fn returns false to stop early.
    template <class Fn> bool for_each_node(Fn fn) const
    {
        uint32_t cells_a[kMaxDepth + 1];
        uint32_t cells_s[kMaxDepth + 1];
        int depth = -1;
        uint32_t pos = 0;
        while (pos + 4 <= size_struct_) {
            uint32_t tok = word(pos);
            if (tok == kBeginNode) {
                if (depth + 1 >= kMaxDepth) {
                    return false;
                }
                ++depth;
                Node n;
                n.offset = pos;
                n.name = reinterpret_cast<const char*>(at(pos + 4));
                n.depth = depth;
                n.addr_cells = depth == 0 ? 2 : cells_a[depth - 1];
                n.size_cells = depth == 0 ? 1 : cells_s[depth - 1];
                // this node's own #address-cells / #size-cells apply to its children
                Prop p;
                cells_a[depth] = get_prop(n, "#address-cells", &p) && p.len == 4 ? be32(p.data) : 2;
                cells_s[depth] = get_prop(n, "#size-cells", &p) && p.len == 4 ? be32(p.data) : 1;
                if (!fn(n)) {
                    return true;
                }
                pos = skip_name(pos + 4);
            } else if (tok == kEndNode) {
                --depth;
                pos += 4;
            } else if (tok == kProp) {
                pos = align4(pos + 12 + word(pos + 4));
            } else if (tok == kNop) {
                pos += 4;
            } else {
                return tok == kEnd;
            }
        }
        return false;
    }

    // Calls fn(prop) for every property of node n (not of its children).
    template <class Fn> void for_each_prop(const Node& n, Fn fn) const
    {
        uint32_t pos = skip_name(n.offset + 4);
        while (pos + 4 <= size_struct_) {
            uint32_t tok = word(pos);
            if (tok == kProp) {
                Prop p;
                p.len = word(pos + 4);
                p.name = string_at(word(pos + 8));
                p.data = at(pos + 12);
                if (!fn(p)) {
                    return;
                }
                pos = align4(pos + 12 + p.len);
            } else if (tok == kNop) {
                pos += 4;
            } else {
                return;   // first child node, END_NODE or END: no more properties
            }
        }
    }

    bool get_prop(const Node& n, const char* name, Prop* out) const
    {
        bool found = false;
        for_each_prop(n, [&](const Prop& p) {
            if (streq(p.name, name)) {
                *out = p;
                found = true;
                return false;
            }
            return true;
        });
        return found;
    }

    // True if the node's "compatible" string list contains compat.
    bool is_compatible(const Node& n, const char* compat) const
    {
        Prop p;
        if (!get_prop(n, "compatible", &p)) {
            return false;
        }
        for (uint32_t i = 0; i < p.len;) {
            const char* s = reinterpret_cast<const char*>(p.data + i);
            if (streq(s, compat)) {
                return true;
            }
            while (i < p.len && p.data[i] != '\0') {
                ++i;
            }
            ++i;
        }
        return false;
    }

    // The i-th string of a string-list property ("" if there is none).
    static const char* string_in(const Prop& p, int index)
    {
        uint32_t i = 0;
        for (int k = 0; k < index && i < p.len; ++k) {
            while (i < p.len && p.data[i] != '\0') {
                ++i;
            }
            ++i;
        }
        return i < p.len ? reinterpret_cast<const char*>(p.data + i) : "";
    }

    // Reads a big-endian number of `cells` 32-bit cells starting at data.
    static uint64_t read_cells(const uint8_t* data, uint32_t cells)
    {
        uint64_t v = 0;
        for (uint32_t c = 0; c < cells; ++c) {
            v = (v << 32) | be32(data + 4 * c);
        }
        return v;
    }

    // Entry i of the node's "reg" property, decoded with the parent's cell counts.
    bool reg(const Node& n, int i, uint64_t* addr, uint64_t* size) const
    {
        Prop p;
        if (!get_prop(n, "reg", &p)) {
            return false;
        }
        uint32_t entry = 4 * (n.addr_cells + n.size_cells);
        if (entry == 0 || (static_cast<uint32_t>(i) + 1) * entry > p.len) {
            return false;
        }
        const uint8_t* d = p.data + static_cast<uint32_t>(i) * entry;
        *addr = read_cells(d, n.addr_cells);
        *size = read_cells(d + 4 * n.addr_cells, n.size_cells);
        return true;
    }

    // First node (after `after`, if given) whose compatible list contains compat.
    bool find_compatible(const char* compat, Node* out, const Node* after = nullptr) const
    {
        bool found = false;
        for_each_node([&](const Node& n) {
            if ((after == nullptr || n.offset > after->offset) && is_compatible(n, compat)) {
                *out = n;
                found = true;
                return false;
            }
            return true;
        });
        return found;
    }

    // Finds a node by absolute path, e.g. "/chosen", "/cpus/cpu@0" or "/pl011" (unit address
    // optional when the name is unique).
    bool find_path(const char* path, Node* out) const
    {
        if (path[0] != '/') {
            return false;
        }
        const char* want[kMaxDepth];
        int nwant = 0;
        // split the path into components without modifying it (component ends at '/' or '\0')
        for (const char* p = path; *p != '\0' && nwant < kMaxDepth;) {
            while (*p == '/') {
                ++p;
            }
            if (*p == '\0') {
                break;
            }
            want[nwant++] = p;
            while (*p != '\0' && *p != '/') {
                ++p;
            }
        }
        int matched = 0;   // components matched along the current branch
        bool found = false;
        for_each_node([&](const Node& n) {
            if (n.depth == 0) {
                if (nwant == 0) {
                    *out = n;
                    found = true;
                    return false;
                }
                return true;
            }
            if (n.depth > matched + 1) {
                return true;              // inside a branch that did not match
            }
            matched = n.depth - 1;        // we left any deeper matched branch
            if (component_matches(n.name, want[matched])) {
                matched = n.depth;
                if (matched == nwant) {
                    *out = n;
                    found = true;
                    return false;
                }
            }
            return true;
        });
        return found;
    }

    // Follows a phandle (the value of an "interrupt-parent" property, for example).
    bool find_phandle(uint32_t ph, Node* out) const
    {
        bool found = false;
        for_each_node([&](const Node& n) {
            Prop p;
            if (get_prop(n, "phandle", &p) && p.len == 4 && be32(p.data) == ph) {
                *out = n;
                found = true;
                return false;
            }
            return true;
        });
        return found;
    }

private:
    const uint8_t* at(uint32_t struct_off) const { return base_ + off_struct_ + struct_off; }
    uint32_t word(uint32_t struct_off) const { return be32(at(struct_off)); }
    const char* string_at(uint32_t off) const
    {
        return reinterpret_cast<const char*>(base_ + off_strings_ + off);
    }
    static uint32_t align4(uint32_t v) { return (v + 3u) & ~3u; }
    uint32_t skip_name(uint32_t name_off) const
    {
        uint32_t i = name_off;
        while (*at(i) != 0) {
            ++i;
        }
        return align4(i + 1);
    }
    static bool component_matches(const char* node, const char* comp)
    {
        // comp ends at '/' or '\0'; it matches "name" or "name@unit" exactly
        const char* a = node;
        const char* b = comp;
        while (*b != '\0' && *b != '/' && *a == *b) {
            ++a;
            ++b;
        }
        bool comp_done = *b == '\0' || *b == '/';
        return comp_done && (*a == '\0' || *a == '@');
    }

    const uint8_t* base_ = nullptr;
    uint32_t total_ = 0, off_struct_ = 0, off_strings_ = 0, version_ = 0, size_struct_ = 0;
};

} // namespace fdt
