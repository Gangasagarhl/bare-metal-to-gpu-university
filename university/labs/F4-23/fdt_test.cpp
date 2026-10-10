// fdt_test.cpp - F4-23: host unit test of fdt.h. It writes a small DTB by hand (big-endian,
// as the format requires), then checks every parser function against the values it wrote.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "fdt.h"

namespace {

class Writer {
public:
    void begin(const std::string& name)
    {
        u32(fdt::kBeginNode);
        bytes(name.c_str(), name.size() + 1);
        pad();
    }
    void end() { u32(fdt::kEndNode); }
    void prop(const std::string& name, const std::vector<uint8_t>& value)
    {
        u32(fdt::kProp);
        u32(static_cast<uint32_t>(value.size()));
        u32(string_offset(name));
        bytes(value.data(), value.size());
        pad();
    }
    void prop_u32s(const std::string& name, const std::vector<uint32_t>& cells)
    {
        std::vector<uint8_t> v;
        for (uint32_t c : cells) {
            for (int s = 24; s >= 0; s -= 8) {
                v.push_back(static_cast<uint8_t>(c >> s));
            }
        }
        prop(name, v);
    }
    void prop_str(const std::string& name, const std::string& s)   // '|' separates list items
    {
        std::vector<uint8_t> v(s.begin(), s.end());
        for (auto& ch : v) {
            if (ch == '|') {
                ch = 0;
            }
        }
        v.push_back(0);
        prop(name, v);
    }
    std::vector<uint8_t> finish()
    {
        u32(fdt::kEnd);
        const uint32_t hdr = 40, rsv = 16;   // header, then one empty reservation entry
        std::vector<uint8_t> out;
        auto put = [&](uint32_t v) {
            for (int s = 24; s >= 0; s -= 8) {
                out.push_back(static_cast<uint8_t>(v >> s));
            }
        };
        uint32_t off_struct = hdr + rsv;
        uint32_t off_strings = off_struct + static_cast<uint32_t>(st_.size());
        uint32_t total = off_strings + static_cast<uint32_t>(strings_.size());
        for (uint32_t v : {fdt::kMagic, total, off_struct, off_strings, hdr, 17u, 16u, 0u,
                           static_cast<uint32_t>(strings_.size()), static_cast<uint32_t>(st_.size())}) {
            put(v);
        }
        out.resize(hdr + rsv, 0);
        out.insert(out.end(), st_.begin(), st_.end());
        out.insert(out.end(), strings_.begin(), strings_.end());
        return out;
    }

private:
    void u32(uint32_t v)
    {
        for (int s = 24; s >= 0; s -= 8) {
            st_.push_back(static_cast<uint8_t>(v >> s));
        }
    }
    void bytes(const void* p, size_t n)
    {
        const auto* b = static_cast<const uint8_t*>(p);
        st_.insert(st_.end(), b, b + n);
    }
    void pad()
    {
        while (st_.size() % 4 != 0) {
            st_.push_back(0);
        }
    }
    uint32_t string_offset(const std::string& name)
    {
        uint32_t off = static_cast<uint32_t>(strings_.size());
        strings_.insert(strings_.end(), name.begin(), name.end());
        strings_.push_back('\0');
        return off;
    }
    std::vector<uint8_t> st_;
    std::vector<char> strings_;
};

int g_failures = 0;

void check(bool ok, const char* what)
{
    std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) {
        ++g_failures;
    }
}

} // namespace

int main()
{
    Writer w;
    w.begin("");
    w.prop_u32s("#address-cells", {2});
    w.prop_u32s("#size-cells", {2});
    w.prop_str("compatible", "test,board|test,family");
    w.begin("chosen");
    w.prop_str("stdout-path", "/soc/uart@10000");
    w.end();
    w.begin("memory@80000000");
    w.prop_str("device_type", "memory");
    w.prop_u32s("reg", {0x0, 0x80000000, 0x0, 0x10000000});
    w.end();
    w.begin("soc");
    w.prop_u32s("#address-cells", {1});
    w.prop_u32s("#size-cells", {1});
    w.begin("intc@20000");
    w.prop_str("compatible", "test,intc");
    w.prop_u32s("reg", {0x20000, 0x1000});
    w.prop_u32s("phandle", {7});
    w.end();
    w.begin("uart@10000");
    w.prop_str("compatible", "ns16550a");
    w.prop_u32s("reg", {0x10000, 0x100});
    w.prop_u32s("interrupt-parent", {7});
    w.end();
    w.end();
    w.end();
    std::vector<uint8_t> blob = w.finish();

    fdt::Blob b;
    check(b.init(blob.data()), "init: magic d00dfeed and version 17 accepted");
    check(b.total_size() == blob.size(), "total_size equals the bytes written");
    std::vector<uint8_t> bad = blob;
    bad[0] = 0;
    fdt::Blob nb;
    check(!nb.init(bad.data()), "init: a damaged magic number is refused");

    int nodes = 0;
    b.for_each_node([&](const fdt::Node&) {
        ++nodes;
        return true;
    });
    check(nodes == 6, "for_each_node visits 6 nodes (root, chosen, memory, soc, intc, uart)");

    fdt::Node n;
    fdt::Prop p;
    check(b.find_path("/chosen", &n) && b.get_prop(n, "stdout-path", &p) &&
              std::strcmp(reinterpret_cast<const char*>(p.data), "/soc/uart@10000") == 0,
          "find_path(/chosen) and stdout-path");
    uint64_t a = 0, s = 0;
    check(b.find_path("/memory", &n) && b.reg(n, 0, &a, &s) && a == 0x80000000 && s == 0x10000000,
          "memory reg read with 2 address cells and 2 size cells");
    check(b.find_compatible("ns16550a", &n) && b.reg(n, 0, &a, &s) && a == 0x10000 && s == 0x100,
          "uart reg read with the parent's 1 + 1 cells");
    check(b.find_path("/soc/uart@10000", &n) && n.depth == 2, "find_path with a unit address");
    check(!b.find_path("/soc/uart@20000", &n), "find_path refuses a wrong unit address");
    check(b.get_prop(n, "interrupt-parent", &p) && b.find_phandle(fdt::be32(p.data), &n) &&
              b.is_compatible(n, "test,intc"),
          "interrupt-parent phandle 7 leads to the interrupt controller");
    check(b.find_path("/", &n) && b.is_compatible(n, "test,family") &&
              b.get_prop(n, "compatible", &p) &&
              std::strcmp(fdt::Blob::string_in(p, 1), "test,family") == 0,
          "root compatible list: second entry found");
    check(!b.find_compatible("no,such-device", &n), "find_compatible reports a missing device");
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
