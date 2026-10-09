// amlcheck.cpp - DR302 F4-11: tests the AML walker (aml.cc) on hand-made byte strings
// whose encoding is worked out in the chapter: PkgLength in 1, 2 and 3 bytes, the name
// prefixes, Package, Buffer, Field units, a skipped If block, and an unknown opcode.
#include "aml.cc"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
int failures = 0;
void check(bool ok, const char* what)
{
    std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok) ++failures;
}

std::vector<uint8_t> table(std::vector<uint8_t> body)          // a 36-byte header, then AML
{
    std::vector<uint8_t> t(36, 0);
    std::memcpy(t.data(), "DSDT", 4);
    t.insert(t.end(), body.begin(), body.end());
    const uint32_t len = static_cast<uint32_t>(t.size());
    std::memcpy(t.data() + 4, &len, 4);
    return t;
}

std::string all_paths;
void collect(const aml::Node& n, void*)
{
    all_paths += std::string(aml::kind_name(n.kind)) + " " + n.path;
    if (n.kind == aml::Kind::FieldUnit) all_paths += " @" + std::to_string(n.bit_offset) + "+" + std::to_string(n.bit_width);
    all_paths += "; ";
}
}  // namespace

int main()
{
    // PkgLength. One byte: 0x3F = 63. Two bytes: 0x4B 0x02 = 0xB | 0x02 << 4 = 43.
    // Three bytes: 0x81 0x34 0x12 = 0x1 | 0x34 << 4 | 0x12 << 12 = 0x12341.
    {
        const uint8_t a[] = {0x3F}, b[] = {0x4B, 0x02}, c[] = {0x81, 0x34, 0x12};
        uint32_t raw;
        const uint8_t* p = a;
        check(aml::pkg_raw(p, a + 1, raw) && raw == 63 && p == a + 1, "PkgLength 1 byte: 0x3F -> 63");
        p = b;
        check(aml::pkg_raw(p, b + 2, raw) && raw == 43 && p == b + 2, "PkgLength 2 bytes: 4B 02 -> 43");
        p = c;
        check(aml::pkg_raw(p, c + 3, raw) && raw == 0x12341 && p == c + 3, "PkgLength 3 bytes: 81 34 12 -> 0x12341");
    }
    // Name strings: root, parent, dual, multi, trailing '_' trimmed.
    {
        char abs[96], raw[96];
        const uint8_t r[] = {'\\', '_', 'S', '5', '_'};
        const uint8_t* p = r;
        check(aml::name_string(p, r + 5, "\\_SB.PCI0", abs, raw, 96) && std::strcmp(abs, "\\_S5") == 0,
              "\\_S5_ -> \\_S5 (root prefix, '_' trimmed)");
        const uint8_t u[] = {'^', 'L', 'N', 'K', 'A'};
        p = u;
        check(aml::name_string(p, u + 5, "\\_SB.PCI0", abs, raw, 96) && std::strcmp(abs, "\\_SB.LNKA") == 0,
              "^LNKA inside \\_SB.PCI0 -> \\_SB.LNKA (parent prefix)");
        const uint8_t d[] = {0x2E, '_', 'S', 'B', '_', 'P', 'C', 'I', '0'};
        p = d;
        check(aml::name_string(p, d + 9, "\\", abs, raw, 96) && std::strcmp(abs, "\\_SB.PCI0") == 0,
              "DualNamePrefix _SB_ PCI0 -> \\_SB.PCI0");
        const uint8_t m[] = {0x5C, 0x2F, 3, '_', 'S', 'B', '_', 'P', 'C', 'I', '0', 'S', '0', '8', '_'};
        p = m;
        check(aml::name_string(p, m + 15, "\\", abs, raw, 96) && std::strcmp(abs, "\\_SB.PCI0.S08") == 0,
              "MultiNamePrefix (3 segments) -> \\_SB.PCI0.S08");
    }
    // Name(_S5_, Package(4){0x05, 0x05, Zero, Zero}): a DSDT as many PCs carry it.
    {
        auto t = table({0x08, '_', 'S', '5', '_', 0x12, 0x08, 0x04, 0x0A, 0x05, 0x0A, 0x05, 0x00, 0x00});
        aml::Node n;
        check(aml::find(t.data(), static_cast<uint32_t>(t.size()), "\\_S5", n), "find \\_S5");
        const uint8_t* p = n.obj;
        aml::Value pkg, e;
        check(aml::value(p, n.end, pkg) && pkg.t == aml::T::Package && pkg.n == 4, "\\_S5 is a Package of 4");
        const uint8_t* q = pkg.p;
        check(aml::value(q, pkg.end, e) && e.t == aml::T::Int && e.i == 5, "element 0: BytePrefix 0x05 -> 5");
        check(q == pkg.p + 2, "element 0 took 2 bytes (0x0A, 0x05)");
    }
    // A Scope with a Device, a Name with a resource template, an OperationRegion and a Field,
    // an If block at definition level, then a Method.
    {
        auto t = table({
            0x10, 0x40, 0x05, '_', 'S', 'B', '_',                         // Scope(\_SB) PkgLength 0x50 = 80 (2 bytes)
              0x5B, 0x82, 0x25, 'D', 'E', 'V', '0',                        // Device(DEV0) PkgLength 37
                0x08, '_', 'C', 'R', 'S',                                  // Name(_CRS, ResourceTemplate
                0x11, 0x0E, 0x0A, 0x0B,                                    //   Buffer PkgLength 14, size 11
                0x89, 0x06, 0x00, 0x09, 0x01, 0x15, 0x00, 0x00, 0x00,      //   Interrupt(ResourceConsumer,
                0x79, 0x00,                                                //   Level, ActiveHigh, Shared){21}
                0x5B, 0x80, 'R', 'E', 'G', '0', 0x01, 0x0B, 0x00, 0x06, 0x0A, 0x04,   // OperationRegion(REG0, SystemIO, 0x600, 4)
              0x5B, 0x81, 0x12, 'R', 'E', 'G', '0', 0x01,                  // Field(REG0, ByteAcc...) PkgLength 18
                0x00, 0x08,                                                //   Offset: 8 reserved bits
                'P', 'W', 'R', 'S', 0x01,                                  //   PWRS, 1 bit
                'S', 'L', 'P', 'S', 0x01,                                  //   SLPS, 1 bit
              0xA0, 0x05, 0x01, 0x70, 0x00, 0x60,                          // If(One){ Store(Zero, Local0) }
              0x14, 0x08, 'D', 'O', 'I', 'T', 0x00, 0xA4, 0x00,            // Method(DOIT){ Return(Zero) }
        });
        all_paths.clear();
        const aml::Result r = aml::walk(t.data(), static_cast<uint32_t>(t.size()), collect, nullptr);
        std::printf("      walk: %s\n", all_paths.c_str());
        check(r.complete && r.skipped == 1, "walk complete, one If block skipped");
        check(all_paths.find("Device \\_SB.DEV0;") != std::string::npos, "Device \\_SB.DEV0 found");
        check(all_paths.find("Field \\_SB.PWRS @8+1;") != std::string::npos, "field unit PWRS at bit 8, 1 bit");
        check(all_paths.find("Field \\_SB.SLPS @9+1;") != std::string::npos, "field unit SLPS at bit 9, 1 bit");
        check(all_paths.find("Method \\_SB.DOIT;") != std::string::npos, "Method \\_SB.DOIT found");
        aml::Node n;
        aml::Value buf;
        uint32_t irq = 0;
        uint8_t fl = 0;
        check(aml::find(t.data(), static_cast<uint32_t>(t.size()), "\\_SB.DEV0._CRS", n), "find \\_SB.DEV0._CRS");
        const uint8_t* p = n.obj;
        check(aml::value(p, n.end, buf) && buf.t == aml::T::Buffer && buf.n == 11, "_CRS is a Buffer of 11 bytes");
        check(aml::first_interrupt(buf, irq, fl) && irq == 21 && fl == 0x09,
              "Extended Interrupt descriptor -> GSI 21, consumer, level, active high, shared (flags 0x09)");
    }
    // Code where a definition is expected (a Store at definition level) stops the walk.
    {
        auto t = table({0x70, 0x00, 0x60});
        const aml::Result r = aml::walk(t.data(), static_cast<uint32_t>(t.size()), nullptr, nullptr);
        check(!r.complete && r.stop_offset == 36 && r.stop_opcode == 0x70, "Store at definition level: walk stops at 0x24, opcode 0x70");
    }
    std::printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
