// fdt_tool.cc - F4-23: host tool built on fdt.h.
//   fdt_tool dump <file.dtb>       print the whole tree (strings as text, other values as cells)
//   fdt_tool checklist <file.dtb>  answer the platform checklist rows a devicetree can answer
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
#include "fdt.h"

namespace {

bool printable_strings(const fdt::Prop& p)
{
    if (p.len == 0 || p.data[p.len - 1] != 0 || p.data[0] == 0) {
        return false;
    }
    for (uint32_t i = 0; i < p.len; ++i) {
        uint8_t c = p.data[i];
        if (c == 0) {
            if (i + 1 < p.len && p.data[i + 1] == 0) {
                return false;       // empty string inside a list: probably binary
            }
        } else if (c < 0x20 || c > 0x7e) {
            return false;
        }
    }
    return true;
}

std::string value_text(const fdt::Prop& p, uint32_t max_cells = 12)
{
    std::string out;
    if (p.len == 0) {
        return "(empty)";
    }
    if (printable_strings(p)) {
        for (int i = 0;; ++i) {
            const char* s = fdt::Blob::string_in(p, i);
            if (*s == '\0') {
                break;
            }
            out += (i ? ", \"" : "\"") + std::string(s) + "\"";
        }
        return out;
    }
    if (p.len % 4 != 0) {
        char buf[16];
        out = "[";
        for (uint32_t i = 0; i < p.len && i < 16; ++i) {
            std::snprintf(buf, sizeof buf, i ? " %02x" : "%02x", p.data[i]);
            out += buf;
        }
        return out + (p.len > 16 ? " ...]" : "]");
    }
    out = "<";
    char buf[16];
    for (uint32_t i = 0; i < p.len / 4 && i < max_cells; ++i) {
        std::snprintf(buf, sizeof buf, i ? " 0x%x" : "0x%x", fdt::be32(p.data + 4 * i));
        out += buf;
    }
    if (p.len / 4 > max_cells) {
        out += " ... (" + std::to_string(p.len / 4) + " cells)";
    }
    return out + ">";
}

void dump(const fdt::Blob& b)
{
    int last_depth = -1;
    b.for_each_node([&](const fdt::Node& n) {
        for (; last_depth >= n.depth; --last_depth) {
            std::printf("%*s};\n", 4 * last_depth, "");
        }
        std::printf("%*s%s {\n", 4 * n.depth, "", n.depth == 0 ? "/" : n.name);
        b.for_each_prop(n, [&](const fdt::Prop& p) {
            std::printf("%*s%s = %s;\n", 4 * (n.depth + 1), "", p.name, value_text(p).c_str());
            return true;
        });
        last_depth = n.depth;
        return true;
    });
    for (; last_depth >= 0; --last_depth) {
        std::printf("%*s};\n", 4 * last_depth, "");
    }
}

std::string prop_or(const fdt::Blob& b, const fdt::Node& n, const char* name, const char* dflt)
{
    fdt::Prop p;
    return b.get_prop(n, name, &p) ? value_text(p) : std::string(dflt);
}

std::string regs(const fdt::Blob& b, const fdt::Node& n)
{
    std::string out;
    uint64_t a = 0, s = 0;
    char buf[64];
    for (int i = 0; b.reg(n, i, &a, &s) && i < 4; ++i) {
        std::snprintf(buf, sizeof buf, "%s0x%llx (size 0x%llx)", i ? ", " : "",
                      static_cast<unsigned long long>(a), static_cast<unsigned long long>(s));
        out += buf;
    }
    return out.empty() ? "(no reg)" : out;
}

void checklist(const fdt::Blob& b)
{
    fdt::Node root, n;
    b.find_path("/", &root);
    std::printf("DTB: version %u, %u bytes\n", b.version(), b.total_size());
    std::printf("model: %s; compatible: %s\n", prop_or(b, root, "model", "(none)").c_str(),
                prop_or(b, root, "compatible", "(none)").c_str());

    // Row 1: CPUs
    int cpus = 0;
    std::string cpu_compat, isa, mmu, enable;
    b.for_each_node([&](const fdt::Node& c) {
        fdt::Prop p;
        if (b.get_prop(c, "device_type", &p) && std::strcmp(reinterpret_cast<const char*>(p.data), "cpu") == 0) {
            if (cpus++ == 0) {
                cpu_compat = prop_or(b, c, "compatible", "(none)");
                isa = prop_or(b, c, "riscv,isa", "");
                mmu = prop_or(b, c, "mmu-type", "(none)");
                enable = prop_or(b, c, "enable-method", "(none in the devicetree)");
            }
        }
        return true;
    });
    std::printf("row 1 CPU: %d cpu node(s); compatible %s", cpus, cpu_compat.c_str());
    if (!isa.empty()) {
        std::printf("; riscv,isa %s; mmu-type %s", isa.c_str(), mmu.c_str());
    }
    std::printf("\n");

    // Row 2: boot handoff information
    if (b.find_path("/chosen", &n)) {
        std::printf("row 2 handoff: /chosen has:");
        b.for_each_prop(n, [&](const fdt::Prop& p) {
            std::printf(" %s", p.name);
            return true;
        });
        std::printf("\n");
    }
    if (b.find_path("/psci", &n)) {
        std::printf("row 2 firmware calls: /psci compatible %s, method %s\n",
                    prop_or(b, n, "compatible", "?").c_str(), prop_or(b, n, "method", "?").c_str());
    } else {
        std::printf("row 2 firmware calls: no /psci node\n");
    }

    // Row 3: memory
    b.for_each_node([&](const fdt::Node& m) {
        fdt::Prop p;
        if (b.get_prop(m, "device_type", &p) && std::strcmp(reinterpret_cast<const char*>(p.data), "memory") == 0) {
            std::printf("row 3 memory: %s reg %s\n", m.name, regs(b, m).c_str());
        }
        return true;
    });
    std::printf("row 3 reserved: %s\n", b.find_path("/reserved-memory", &n) ? "a /reserved-memory node exists"
                                                                              : "no /reserved-memory node");

    // Row 4: interrupt controllers (every node with an "interrupt-controller" property)
    b.for_each_node([&](const fdt::Node& ic) {
        fdt::Prop p;
        if (b.get_prop(ic, "interrupt-controller", &p)) {
            std::string c = prop_or(b, ic, "compatible", "(no compatible)");
            if (c.find("cpu-intc") != std::string::npos) {
                return true;   // one per hart on RISC-V: summarised in row 1
            }
            std::printf("row 4 interrupt controller: %s compatible %s reg %s\n", ic.name, c.c_str(),
                        regs(b, ic).c_str());
        }
        return true;
    });

    // Row 5: timer
    if (b.find_compatible("arm,armv8-timer", &n)) {
        std::printf("row 5 timer: %s compatible %s interrupts %s\n", n.name,
                    prop_or(b, n, "compatible", "?").c_str(), prop_or(b, n, "interrupts", "?").c_str());
    }
    if (b.find_path("/cpus", &n)) {
        fdt::Prop p;
        if (b.get_prop(n, "timebase-frequency", &p)) {
            std::printf("row 5 timer: /cpus timebase-frequency %u Hz\n", fdt::be32(p.data));
        }
    }

    // Row 6: console
    std::string out_path;
    if (b.find_path("/chosen", &n)) {
        fdt::Prop p;
        if (b.get_prop(n, "stdout-path", &p)) {
            out_path = reinterpret_cast<const char*>(p.data);
            out_path = out_path.substr(0, out_path.find(':'));   // drop ":115200n8"-style options
        }
    }
    if (!out_path.empty() && b.find_path(out_path.c_str(), &n)) {
        std::printf("row 6 console: %s compatible %s reg %s interrupts %s\n", out_path.c_str(),
                    prop_or(b, n, "compatible", "?").c_str(), regs(b, n).c_str(),
                    prop_or(b, n, "interrupts", "(none)").c_str());
    } else {
        std::printf("row 6 console: no usable stdout-path\n");
    }

    // Row 7: storage paths
    int mmio = 0;
    fdt::Node first;
    b.for_each_node([&](const fdt::Node& v) {
        if (b.is_compatible(v, "virtio,mmio")) {
            if (mmio++ == 0) {
                first = v;
            }
        }
        return true;
    });
    if (mmio > 0) {
        std::printf("row 7 storage: %d virtio,mmio slot(s), first %s reg %s\n", mmio, first.name,
                    regs(b, first).c_str());
    }
    if (b.find_compatible("pci-host-ecam-generic", &n)) {
        std::printf("row 7 storage: PCI host bridge %s compatible %s\n", n.name,
                    prop_or(b, n, "compatible", "?").c_str());
    }

    // Row 8: SMP start method
    std::printf("row 8 SMP: cpu enable-method %s\n", enable.c_str());

    // Row 9: everything else, by compatible string
    std::set<std::string> other;
    b.for_each_node([&](const fdt::Node& o) {
        fdt::Prop p;
        if (o.depth > 0 && b.get_prop(o, "compatible", &p)) {
            other.insert(fdt::Blob::string_in(p, 0));
        }
        return true;
    });
    std::printf("row 9 other compatible strings (%zu):", other.size());
    for (const auto& s : other) {
        std::printf(" %s", s.c_str());
    }
    std::printf("\n");
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: fdt_tool dump|checklist <file.dtb>\n");
        return 2;
    }
    std::ifstream f(argv[2], std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    fdt::Blob b;
    if (data.size() < 40 || !b.init(data.data()) || b.total_size() > data.size()) {
        std::fprintf(stderr, "%s: not a devicetree blob this tool understands\n", argv[2]);
        return 1;
    }
    if (std::strcmp(argv[1], "dump") == 0) {
        dump(b);
    } else {
        checklist(b);
    }
    return 0;
}
