// fdtdump.cc - F4-31: print a devicetree blob as text, or as a device table.
//
//   fdtdump <file.dtb>            the whole tree, properties decoded as strings or cells
//   fdtdump --devices <file.dtb>  one line per node that has "reg": path, compatible, CPU address
//
// Host program (g++ with sanitizers, see run.sh). The reading is done by fdt.h, the same header
// the bare-metal kernel uses, so this tool is also the kernel parser's test on real input.
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "fdt.h"

namespace {

// A property is shown as text if it is a list of printable NUL-terminated strings.
bool printable_strings(const fdt::Prop& p)
{
    if (p.len == 0 || p.data[p.len - 1] != '\0') {
        return false;
    }
    bool start = true;
    for (uint32_t i = 0; i < p.len; ++i) {
        unsigned char c = p.data[i];
        if (c == '\0') {
            if (start) {
                return false;    // empty string inside the list
            }
            start = true;
        } else if (!std::isprint(c)) {
            return false;
        } else {
            start = false;
        }
    }
    return true;
}

std::string value(const fdt::Prop& p)
{
    std::string out;
    char buf[32];
    if (p.len == 0) {
        return "";
    }
    if (printable_strings(p)) {
        out = " = \"";
        for (uint32_t i = 0; i + 1 < p.len; ++i) {
            out += p.data[i] == '\0' ? std::string("\", \"") : std::string(1, char(p.data[i]));
        }
        return out + "\"";
    }
    if (p.len % 4 == 0) {
        out = " = <";
        uint32_t shown = p.len / 4 > 16 ? 16 : p.len / 4;
        for (uint32_t i = 0; i < shown; ++i) {
            std::snprintf(buf, sizeof buf, "%s0x%x", i ? " " : "", p.u32(i));
            out += buf;
        }
        if (shown < p.len / 4) {
            std::snprintf(buf, sizeof buf, " ... (%u cells)", p.len / 4);
            out += buf;
        }
        return out + ">";
    }
    std::snprintf(buf, sizeof buf, " = [%u bytes]", p.len);
    return buf;
}

void dump_tree(const fdt::Tree& t)
{
    int depth = 0;
    int prev_depth = 0;
    for (int n = t.root(); n != fdt::kNone; n = t.next_node(n, &depth)) {
        for (int d = prev_depth; d >= depth && n != t.root(); --d) {
            std::printf("%*s};\n", 4 * d, "");
        }
        prev_depth = depth;
        std::printf("%*s%s {\n", 4 * depth, "", depth == 0 ? "/" : t.name(n));
        fdt::Prop p;
        for (int i = 0; t.prop_at(n, i, p); ++i) {
            std::printf("%*s%s%s;\n", 4 * depth + 4, "", p.name, value(p).c_str());
        }
    }
    for (int d = prev_depth; d >= 0; --d) {
        std::printf("%*s};\n", 4 * d, "");
    }
}

void dump_devices(const fdt::Tree& t)
{
    std::printf("%-34s %-28s %-12s %s\n", "node", "first compatible", "address", "size");
    for (int n = t.root(); n != fdt::kNone; n = t.next_node(n)) {
        uint64_t a = 0;
        uint64_t s = 0;
        if (!t.reg(n, 0, a, s)) {
            continue;
        }
        char path[128];
        t.path(n, path, sizeof path);
        fdt::Prop c;
        const char* comp = t.prop(n, "compatible", c) ? reinterpret_cast<const char*>(c.data) : "-";
        bool ok = t.translate(n, a);
        std::printf("%-34s %-28s 0x%-10llx 0x%llx%s\n", path, comp, static_cast<unsigned long long>(a),
                    static_cast<unsigned long long>(s), ok ? "" : "  (not translatable: no ranges)");
    }
}

}  // namespace

int main(int argc, char** argv)
{
    bool devices = argc == 3 && std::string(argv[1]) == "--devices";
    if (argc != 2 && !devices) {
        std::fprintf(stderr, "usage: fdtdump [--devices] file.dtb\n");
        return 2;
    }
    std::ifstream in(argv[argc - 1], std::ios::binary);
    std::vector<uint8_t> blob((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (blob.size() < 40) {
        std::fprintf(stderr, "cannot read %s\n", argv[argc - 1]);
        return 1;
    }
    fdt::Tree t;
    const char* err = nullptr;
    if (!t.init(blob.data(), &err) || t.total_size() > blob.size()) {
        std::fprintf(stderr, "not a usable DTB: %s\n", err != nullptr ? err : "file shorter than totalsize");
        return 1;
    }
    if (devices) {
        dump_devices(t);
    } else {
        std::printf("// DTB version %u, totalsize %u bytes, structure block %u bytes, strings %u bytes\n",
                    t.version(), t.total_size(), t.struct_size(), t.strings_size());
        dump_tree(t);
    }
    return 0;
}
