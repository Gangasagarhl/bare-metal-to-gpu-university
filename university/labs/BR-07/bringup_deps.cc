// bringup_deps.cc - BR-07: the dependency inventory of a board, read from its devicetree.
//
//   bringup_deps <file.dtb>
//
// For every node with a compatible string: its status, its first reg entry translated to a
// CPU address through every ranges above it, and what it needs before it can work: clocks,
// resets, pins, supplies, power domains, interrupt parent, GPIOs, DMA channels, and whether it
// is marked dma-coherent. Then the providers, and a bring-up order in which every provider
// comes before its consumers. This is the input to the bring-up plan, not the plan itself:
// the tree cannot say whether firmware left a clock running, which pins carry which voltage,
// or how the board recovers from a bad image.
//
// The DTB reader is F4-23's fdt.h (DR402), unchanged. Property meanings after the Devicetree
// Specification and the clock, reset, pinctrl, regulator and power-domain bindings (title
// only, pending verification).
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../F4-23/fdt.h"

namespace {

struct Info {
    std::string path;
    std::map<std::string, uint32_t> cells;   // "#clock-cells" -> 1, ...
};

struct Device {
    std::string path, compat, status;
    std::string addr = "-";
    std::vector<std::string> needs;          // "clock clock-controller@10000000 3", ...
    std::set<std::string> providers;         // paths of the nodes it depends on
    bool coherent = false;
};

uint32_t prop_u32(const fdt::Blob& t, const fdt::Node& n, const char* name, uint32_t dflt)
{
    fdt::Prop p;
    return t.get_prop(n, name, &p) && p.len == 4 ? fdt::be32(p.data) : dflt;
}

std::string hex(uint64_t v)
{
    char b[24];
    std::snprintf(b, sizeof b, "0x%llx", static_cast<unsigned long long>(v));
    return b;
}

// A short name for a node: its last path component, or the last two when the last one has no
// unit address (several nodes may be called "interrupt-controller").
std::string last(const std::string& path)
{
    size_t cut = path.rfind('/');
    if (path.find('@', cut) == std::string::npos && cut != 0 && cut != std::string::npos) {
        cut = path.rfind('/', cut - 1);
    }
    return path.substr(cut + 1);
}

// Translate a bus address up through the ranges of every ancestor (stack[1..depth-1]).
bool translate(const fdt::Blob& t, const std::vector<fdt::Node>& stack, int depth, uint64_t* a)
{
    for (int d = depth - 1; d >= 1; --d) {
        const fdt::Node& bus = stack[static_cast<size_t>(d)];
        fdt::Prop r;
        if (!t.get_prop(bus, "ranges", &r)) {
            return false;                      // not memory-mapped from above
        }
        if (r.len == 0) {
            continue;                          // empty ranges: identity
        }
        uint32_t ca = prop_u32(t, bus, "#address-cells", 2);
        uint32_t pa = bus.addr_cells;          // the bus's parent's #address-cells
        uint32_t cs = prop_u32(t, bus, "#size-cells", 1);
        uint32_t entry = 4 * (ca + pa + cs);
        bool hit = false;
        for (uint32_t off = 0; entry != 0 && off + entry <= r.len; off += entry) {
            uint64_t child = fdt::Blob::read_cells(r.data + off, ca);
            uint64_t parent = fdt::Blob::read_cells(r.data + off + 4 * ca, pa);
            uint64_t size = fdt::Blob::read_cells(r.data + off + 4 * (ca + pa), cs);
            if (*a >= child && *a - child < size) {
                *a = parent + (*a - child);
                hit = true;
                break;
            }
        }
        if (!hit) {
            return false;
        }
    }
    return true;
}

// A list of (phandle, args...) entries, the number of args given by the provider's #<kind>-cells.
using Provides = std::map<std::string, std::set<std::string>>;   // provider path -> kinds

void phandle_list(const fdt::Prop& p, const char* kind, const char* cells_name,
                  const std::map<uint32_t, Info>& by_ph, Device& d, Provides& provides)
{
    for (uint32_t off = 0; off + 4 <= p.len;) {
        uint32_t ph = fdt::be32(p.data + off);
        off += 4;
        auto it = by_ph.find(ph);
        if (it == by_ph.end()) {
            d.needs.push_back(std::string(kind) + " <unknown phandle " + hex(ph) + ">");
            return;
        }
        uint32_t n = 0;
        if (cells_name != nullptr) {
            auto c = it->second.cells.find(cells_name);
            n = c == it->second.cells.end() ? 0 : c->second;
        }
        std::string s = std::string(kind) + " " + last(it->second.path);
        for (uint32_t i = 0; i < n && off + 4 <= p.len; ++i, off += 4) {
            s += " " + std::to_string(fdt::be32(p.data + off));
        }
        d.needs.push_back(s);
        d.providers.insert(it->second.path);
        provides[it->second.path].insert(kind);
    }
}

bool ends_with(const std::string& s, const std::string& tail)
{
    return s.size() >= tail.size() && s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: bringup_deps <file.dtb>\n");
        return 2;
    }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    fdt::Blob t;
    if (buf.size() < 40 || !t.init(buf.data()) || t.total_size() > buf.size()) {
        std::fprintf(stderr, "%s: not a valid DTB\n", argv[1]);
        return 1;
    }
    static const char* kCells[] = {"#clock-cells", "#reset-cells", "#power-domain-cells",
                                   "#gpio-cells", "#interrupt-cells", "#dma-cells"};

    // Pass 1: every node with a phandle, by phandle, with its #...-cells values.
    std::map<uint32_t, Info> by_ph;
    std::vector<fdt::Node> stack;
    auto path_of = [&](int depth) {
        std::string p;
        for (int d = 1; d <= depth; ++d) {
            p += "/";
            p += stack[static_cast<size_t>(d)].name;
        }
        return p.empty() ? std::string("/") : p;
    };
    t.for_each_node([&](const fdt::Node& n) {
        stack.resize(static_cast<size_t>(n.depth) + 1);
        stack[static_cast<size_t>(n.depth)] = n;
        uint32_t ph = prop_u32(t, n, "phandle", 0);
        if (ph != 0) {
            Info& i = by_ph[ph];
            i.path = path_of(n.depth);
            for (const char* c : kCells) {
                fdt::Prop p;
                if (t.get_prop(n, c, &p) && p.len == 4) {
                    i.cells[c] = fdt::be32(p.data);
                }
            }
        }
        return true;
    });

    // Pass 2: every device, its address and its needs.
    std::vector<Device> devs;
    Provides provides;
    t.for_each_node([&](const fdt::Node& n) {
        stack.resize(static_cast<size_t>(n.depth) + 1);
        stack[static_cast<size_t>(n.depth)] = n;
        fdt::Prop p;
        if (n.depth == 0 || !t.get_prop(n, "compatible", &p)) {
            return true;
        }
        Device d;
        d.path = path_of(n.depth);
        d.compat = fdt::Blob::string_in(p, 0);
        d.status = t.get_prop(n, "status", &p) ? reinterpret_cast<const char*>(p.data) : "(none)";
        uint64_t a = 0;
        uint64_t s = 0;
        if (t.reg(n, 0, &a, &s)) {
            d.addr = translate(t, stack, n.depth, &a) ? hex(a) : "not mapped";
        }
        t.for_each_prop(n, [&](const fdt::Prop& q) {
            std::string name = q.name;
            if (name == "clocks") {
                phandle_list(q, "clock", "#clock-cells", by_ph, d, provides);
            } else if (name == "resets") {
                phandle_list(q, "reset", "#reset-cells", by_ph, d, provides);
            } else if (name == "power-domains") {
                phandle_list(q, "power-domain", "#power-domain-cells", by_ph, d, provides);
            } else if (name.rfind("pinctrl-", 0) == 0 && name != "pinctrl-names") {
                phandle_list(q, "pins", nullptr, by_ph, d, provides);
            } else if (ends_with(name, "-supply")) {
                phandle_list(q, "supply", nullptr, by_ph, d, provides);
            } else if (name == "interrupt-parent") {
                phandle_list(q, "irq-parent", nullptr, by_ph, d, provides);
            } else if (name == "interrupts-extended") {
                phandle_list(q, "irq", "#interrupt-cells", by_ph, d, provides);
            } else if (name == "gpios" || ends_with(name, "-gpios")) {
                phandle_list(q, "gpio", "#gpio-cells", by_ph, d, provides);
            } else if (name == "dmas") {
                phandle_list(q, "dma", "#dma-cells", by_ph, d, provides);
            } else if (name == "dma-coherent") {
                d.coherent = true;
            }
            return true;
        });
        // "interrupts" without a local parent: the interrupt-parent is inherited from an ancestor
        if (t.get_prop(n, "interrupts", &p) && !t.get_prop(n, "interrupt-parent", &p) &&
            !t.get_prop(n, "interrupts-extended", &p)) {
            for (int up = n.depth - 1; up >= 0; --up) {
                if (t.get_prop(stack[static_cast<size_t>(up)], "interrupt-parent", &p)) {
                    phandle_list(p, "irq-parent", nullptr, by_ph, d, provides);
                    break;
                }
            }
        }
        devs.push_back(d);
        return true;
    });

    fdt::Node root;
    fdt::Prop p;
    if (t.find_path("/", &root) && t.get_prop(root, "model", &p)) {
        std::printf("model: %s\n", reinterpret_cast<const char*>(p.data));
    }
    fdt::Node chosen;
    if (t.find_path("/chosen", &chosen) && t.get_prop(chosen, "stdout-path", &p)) {
        std::printf("console (/chosen stdout-path): %s\n", reinterpret_cast<const char*>(p.data));
    }
    std::printf("\n%-42s %-26s %-9s %-12s %s\n", "node", "first compatible", "status", "CPU address",
                "needs");
    int n_clk = 0, n_rst = 0, n_pin = 0, n_sup = 0, n_pd = 0, n_coh = 0, n_dis = 0;
    for (const Device& d : devs) {
        std::string needs;
        bool clk = false, rst = false, pin = false, sup = false, pd = false;
        for (const std::string& s : d.needs) {
            needs += (needs.empty() ? "" : "; ") + s;
            clk |= s.rfind("clock ", 0) == 0;
            rst |= s.rfind("reset ", 0) == 0;
            pin |= s.rfind("pins ", 0) == 0;
            sup |= s.rfind("supply ", 0) == 0;
            pd |= s.rfind("power-domain ", 0) == 0;
        }
        if (d.coherent) {
            needs += (needs.empty() ? "" : "; ") + std::string("dma-coherent");
        }
        n_clk += clk;
        n_rst += rst;
        n_pin += pin;
        n_sup += sup;
        n_pd += pd;
        n_coh += d.coherent;
        n_dis += d.status == "disabled";
        std::printf("%-42s %-26s %-9s %-12s %s\n", d.path.c_str(), d.compat.c_str(), d.status.c_str(),
                    d.addr.c_str(), needs.empty() ? "-" : needs.c_str());
    }

    std::printf("\nproviders (what other nodes point at):\n");
    for (const auto& [path, kinds] : provides) {
        std::string k;
        for (const std::string& s : kinds) {
            k += (k.empty() ? "" : ", ") + s;
        }
        std::printf("  %-40s provides: %s\n", path.c_str(), k.c_str());
    }

    // Bring-up order: a node's stage is one more than the highest stage of its providers.
    std::map<std::string, int> stage;
    for (const Device& d : devs) {
        stage[d.path] = 0;
    }
    for (size_t round = 0; round < devs.size(); ++round) {
        bool changed = false;
        for (const Device& d : devs) {
            for (const std::string& pr : d.providers) {
                auto it = stage.find(pr);
                int need = (it == stage.end() ? 0 : it->second) + 1;
                if (pr != d.path && stage[d.path] < need) {
                    stage[d.path] = need;
                    changed = true;
                }
            }
        }
        if (!changed) {
            break;
        }
    }
    int top = 0;
    for (const auto& [path, s] : stage) {
        top = s > top ? s : top;
    }
    std::printf("\nbring-up order (providers first; nodes without a provider are stage 0):\n");
    for (int s = 0; s <= top; ++s) {
        std::printf("  stage %d:", s);
        for (const Device& d : devs) {
            if (stage[d.path] == s) {
                std::printf(" %s", last(d.path).c_str());
            }
        }
        std::printf("\n");
    }
    std::printf("\nsummary: %zu nodes with compatible; need a clock %d, a reset %d, pins %d, a supply %d, "
                "a power domain %d; dma-coherent %d; status disabled %d\n",
                devs.size(), n_clk, n_rst, n_pin, n_sup, n_pd, n_coh, n_dis);
    return 0;
}
