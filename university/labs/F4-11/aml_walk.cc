// aml_walk.cc - DR302 F4-11 host tool: walks the namespace of a DSDT that the lab kernel
// dumped from QEMU, prints it as a tree, then answers the three questions of the chapter:
// what does \_S5 hold, what does the APIC-mode PCI routing table say, which GSI does each
// interrupt link device offer. Uses the same aml.cc as the kernel.
// usage: aml_walk dsdt.aml [--tree]
#include "aml.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
struct Tree { bool print; unsigned per_kind[13]; };

void show(const aml::Node& n, void* ctx)
{
    auto* t = static_cast<Tree*>(ctx);
    ++t->per_kind[static_cast<int>(n.kind)];
    if (!t->print) return;
    std::printf("%5x %*s%-15s %s", n.offset, 2 * n.depth, "", aml::kind_name(n.kind), n.path);
    if (n.kind == aml::Kind::Method) std::printf("  (%u args)", n.obj[0] & 7);
    if (n.kind == aml::Kind::FieldUnit) std::printf("  bits %u+%u of %s", n.bit_offset, n.bit_width, n.region);
    if (n.kind == aml::Kind::Region) {
        const uint8_t* p = n.obj + 1;
        aml::Value off, len;
        aml::value(p, n.end, off);
        aml::value(p, n.end, len);
        static const char* const space[] = {"SystemMemory", "SystemIO", "PCI_Config", "EmbeddedControl", "SMBus"};
        std::printf("  %s 0x%llx, %llu bytes", n.obj[0] < 5 ? space[n.obj[0]] : "other",
                    static_cast<unsigned long long>(off.i), static_cast<unsigned long long>(len.i));
    }
    if (n.kind == aml::Kind::Name) {
        const uint8_t* p = n.obj;
        aml::Value v;
        aml::value(p, n.end, v);
        if (v.t == aml::T::Int) std::printf("  = 0x%llx", static_cast<unsigned long long>(v.i));
        if (v.t == aml::T::String) std::printf("  = \"%.*s\"", static_cast<int>(v.n), v.p);
        if (v.t == aml::T::Buffer) std::printf("  = Buffer, %u bytes", v.n);
        if (v.t == aml::T::Package) std::printf("  = Package, %u elements", v.n);
    }
    std::printf("\n");
}

std::string eisa(uint32_t id)                       // compressed EISA id -> "PNP0C0F"
{
    const uint32_t b = ((id & 0xFF) << 24) | ((id & 0xFF00) << 8) | ((id >> 8) & 0xFF00) | (id >> 24);
    char s[8];
    s[0] = static_cast<char>('@' + ((b >> 26) & 0x1F));
    s[1] = static_cast<char>('@' + ((b >> 21) & 0x1F));
    s[2] = static_cast<char>('@' + ((b >> 16) & 0x1F));
    std::snprintf(s + 3, 5, "%04X", b & 0xFFFF);
    return s;
}
}  // namespace

int main(int argc, char** argv)
{
    if (argc < 2) { std::fprintf(stderr, "usage: aml_walk dsdt.aml [--tree]\n"); return 2; }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> t((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (t.size() < 36 || std::memcmp(t.data(), "DSDT", 4) != 0) { std::printf("not a DSDT\n"); return 1; }
    uint32_t len;
    std::memcpy(&len, t.data() + 4, 4);
    uint8_t sum = 0;
    for (uint32_t i = 0; i < len && i < t.size(); ++i) sum = static_cast<uint8_t>(sum + t[i]);
    std::printf("DSDT: %u bytes (file %zu), revision %u, checksum %s, OEM \"%.6s\" \"%.8s\"\n", len, t.size(), t[8],
                sum == 0 ? "ok" : "BAD", reinterpret_cast<char*>(&t[10]), reinterpret_cast<char*>(&t[16]));

    const bool tree = argc > 2 && std::strcmp(argv[2], "--tree") == 0;
    Tree tr{tree, {}};
    if (tree) std::printf("offset  kind            path\n");
    const aml::Result r = aml::walk(t.data(), len, show, &tr);
    std::printf("walk: %u named objects, %u If/Else/While blocks at definition level skipped, %s",
                r.nodes, r.skipped, r.complete ? "complete" : "STOPPED");
    if (!r.complete) std::printf(" at offset 0x%x (opcode 0x%02x)", r.stop_offset, r.stop_opcode);
    std::printf("\n      ");
    for (int k = 0; k < 13; ++k)
        if (tr.per_kind[k]) std::printf("%s %u  ", aml::kind_name(static_cast<aml::Kind>(k)), tr.per_kind[k]);
    std::printf("\n");
    if (tree) return r.complete ? 0 : 1;

    // 1. \_S5: the value to write into SLP_TYP for soft-off.
    aml::Node n;
    if (aml::find(t.data(), len, "\\_S5", n)) {
        const uint8_t* p = n.obj;
        aml::Value pkg, e;
        aml::value(p, n.end, pkg);
        std::printf("\\_S5 at offset 0x%x: Package of %u elements:", n.offset, pkg.n);
        const uint8_t* q = pkg.p;
        for (uint32_t i = 0; i < pkg.n && aml::value(q, pkg.end, e); ++i)
            std::printf(" 0x%llx", static_cast<unsigned long long>(e.i));
        std::printf("   (SLP_TYPa, SLP_TYPb, reserved...)\n  raw bytes:");
        for (const uint8_t* b = t.data() + n.offset; b < pkg.end; ++b) std::printf(" %02x", *b);
        std::printf("\n");
    } else {
        std::printf("\\_S5: not found as a static Name\n");
    }

    // 2. _PRT: QEMU's \_SB.PCI0._PRT is a Method that returns PRTP (PIC mode) or PRTA
    //    (APIC mode); a walker cannot run it, so read the two static tables directly.
    for (const char* name : {"\\_SB.PCI0._PRT", "\\_SB.PCI0.PRTA"}) {
        if (!aml::find(t.data(), len, name, n)) { std::printf("%s: not found\n", name); continue; }
        if (n.kind == aml::Kind::Method) {
            std::printf("%s: a Method (%u args): only an interpreter can say what it returns\n", name, n.obj[0] & 7);
            continue;
        }
        const uint8_t* p = n.obj;
        aml::Value pkg, entry, e;
        aml::value(p, n.end, pkg);
        std::printf("%s: Package of %u routing entries; the first 8 (device, pin -> source):\n", name, pkg.n);
        const uint8_t* q = pkg.p;
        for (uint32_t i = 0; i < pkg.n && i < 8 && aml::value(q, pkg.end, entry); ++i) {
            const uint8_t* r2 = entry.p;
            uint64_t f[4] = {};
            char src[96] = "";
            for (uint32_t k = 0; k < 4 && aml::value(r2, entry.end, e); ++k) {
                if (e.t == aml::T::NameRef) std::snprintf(src, sizeof src, "%s", e.name);
                else f[k] = e.i;
            }
            std::printf("  address 0x%08llx = device %llu, pin %c (%llu) -> %s, index %llu\n",
                        static_cast<unsigned long long>(f[0]), static_cast<unsigned long long>(f[0] >> 16),
                        static_cast<char>('A' + f[1]), static_cast<unsigned long long>(f[1]), src,
                        static_cast<unsigned long long>(f[3]));
        }
    }

    // 3. The link devices GSIA..GSIH: _HID and the interrupt their _PRS offers.
    for (char c = 'A'; c <= 'H'; ++c) {
        char path[32];
        std::snprintf(path, sizeof path, "\\_SB.GSI%c._PRS", c);
        if (!aml::find(t.data(), len, path, n)) continue;
        if (n.kind != aml::Kind::Name) { std::printf("%s: a %s\n", path, aml::kind_name(n.kind)); continue; }
        const uint8_t* p = n.obj;
        aml::Value buf;
        aml::value(p, n.end, buf);
        uint32_t irq = 0;
        uint8_t fl = 0;
        std::string hid = "?";
        char hp[32];
        std::snprintf(hp, sizeof hp, "\\_SB.GSI%c._HID", c);
        aml::Node h;
        if (aml::find(t.data(), len, hp, h)) {
            const uint8_t* q = h.obj;
            aml::Value v;
            aml::value(q, h.end, v);
            if (v.t == aml::T::Int) hid = eisa(static_cast<uint32_t>(v.i));
        }
        if (aml::first_interrupt(buf, irq, fl))
            std::printf("\\_SB.GSI%c: _HID %s, _PRS offers interrupt %u, %s, active %s%s\n", c, hid.c_str(), irq,
                        (fl & 2) ? "edge" : "level", (fl & 4) ? "low" : "high", (fl & 8) ? ", shared" : "");
    }
    return r.complete ? 0 : 1;
}
