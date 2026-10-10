// hid_parse.cc - see hid_parse.h.
#include "hid_parse.h"
#include <cstdio>

namespace hid {

namespace {
struct Globals {
    uint16_t usage_page = 0;
    int32_t logical_min = 0, logical_max = 0;
    uint32_t report_size = 0, report_count = 0;
    uint8_t report_id = 0;
};
uint32_t uval(const uint8_t* p, int n) { uint32_t v = 0; for (int i = 0; i < n; ++i) v |= uint32_t(p[i]) << (8 * i); return v; }
int32_t sval(const uint8_t* p, int n)
{
    if (n == 0) return 0;
    const uint32_t v = uval(p, n);
    if (n == 4) return static_cast<int32_t>(v);
    const uint32_t sign = 1u << (8 * n - 1);
    return static_cast<int32_t>((v ^ sign) - sign);          // sign-extend 1 or 2 bytes
}
} // namespace

Parsed parse(const std::vector<uint8_t>& d)
{
    Parsed out;
    Globals g;
    std::vector<Globals> stack;
    std::vector<uint32_t> usages;
    uint32_t umin = 0, umax = 0;
    bool have_range = false;
    int depth = 0;
    std::vector<std::pair<uint8_t, uint32_t>> offset;    // bits used so far, per report ID
    auto bits_for = [&](uint8_t id) -> uint32_t& {
        for (auto& e : offset) if (e.first == id) return e.second;
        offset.push_back({id, 0});
        return offset.back().second;
    };
    static const int kSize[4] = {0, 1, 2, 4};
    size_t i = 0;
    while (i < d.size()) {
        const uint8_t prefix = d[i];
        if (prefix == 0xFE) {                               // long item
            if (i + 1 >= d.size()) { out.error = "truncated long item"; break; }
            i += 3 + d[i + 1];
            continue;
        }
        const int n = kSize[prefix & 3];
        if (i + 1 + n > d.size()) { out.error = "item runs past the end"; break; }
        const uint8_t* p = &d[i + 1];
        const uint8_t type = (prefix >> 2) & 3, tag = prefix >> 4;
        {
            static const char* const kType[4] = {"main", "global", "local", "reserved"};
            char buf[64];
            int k = std::snprintf(buf, sizeof buf, "%3zu:", i);
            for (int b = 0; b <= n; ++b) k += std::snprintf(buf + k, sizeof buf - k, " %02x", d[i + b]);
            std::snprintf(buf + k, sizeof buf - k, "%*s%-6s tag 0x%x", 16 - 3 * (n + 1), "", kType[type], tag);
            out.items.push_back(buf);
        }
        if (type == 1) {                                    // global
            switch (tag) {
            case 0x0: g.usage_page = static_cast<uint16_t>(uval(p, n)); break;
            case 0x1: g.logical_min = sval(p, n); break;
            case 0x2:
                g.logical_max = sval(p, n);
                // "25 ff" after "15 00": a 1-byte maximum of 0xFF read signed is -1, which cannot
                // be above a minimum of 0. Read it unsigned, and say so.
                if (g.logical_min >= 0 && g.logical_max < 0 && n < 4) {
                    g.logical_max = static_cast<int32_t>(uval(p, n));
                    out.warnings.push_back("logical maximum " + std::to_string(g.logical_max) +
                                           " read unsigned (signed value was below the minimum)");
                }
                break;
            case 0x7: g.report_size = uval(p, n); break;
            case 0x8: g.report_id = static_cast<uint8_t>(uval(p, n)); break;
            case 0x9: g.report_count = uval(p, n); break;
            case 0xA: stack.push_back(g); break;           // Push
            case 0xB: if (!stack.empty()) { g = stack.back(); stack.pop_back(); } break;   // Pop
            default: break;                                 // physical min/max, unit, exponent
            }
        } else if (type == 2) {                             // local
            switch (tag) {
            case 0x0: usages.push_back(uval(p, n)); break;
            case 0x1: umin = uval(p, n); have_range = true; break;
            case 0x2: umax = uval(p, n); have_range = true; break;
            default: break;
            }
        } else if (type == 0) {                             // main
            switch (tag) {
            case 0x8: {                                     // Input
                const uint32_t flags = uval(p, n);
                Field f{g.report_id, g.usage_page, 0, 0, g.logical_min, g.logical_max,
                        bits_for(g.report_id), g.report_size, g.report_count, flags, depth};
                if (have_range) { f.usage = umin; f.usage_max = umax; }
                else if (!usages.empty()) { f.usage = usages.front(); f.usage_max = usages.back(); }
                out.fields.push_back(f);
                bits_for(g.report_id) += g.report_size * g.report_count;
                break;
            }
            case 0xA: {                                     // Collection
                char name[64];
                std::snprintf(name, sizeof name, "depth %d: type %u, usage %s", depth, n ? p[0] : 0,
                              usage_name(g.usage_page, usages.empty() ? 0 : usages.front()).c_str());
                out.collections.push_back(name);
                ++depth;
                break;
            }
            case 0xC: --depth; break;                       // End Collection
            default: break;                                 // Output, Feature: not reports we read
            }
            usages.clear(); have_range = false; umin = umax = 0;   // locals end at a main item
        }
        i += 1 + n;
    }
    if (out.error.empty() && depth != 0) out.error = "collections not closed";
    for (auto& e : offset) out.reports.push_back({e.first, e.second});
    return out;
}

std::string usage_name(uint16_t page, uint32_t usage)
{
    if (usage > 0xFFFF) { page = static_cast<uint16_t>(usage >> 16); usage &= 0xFFFF; }
    if (page == 0x01) {
        switch (usage) {
        case 0x01: return "Pointer"; case 0x02: return "Mouse"; case 0x06: return "Keyboard";
        case 0x30: return "X"; case 0x31: return "Y"; case 0x38: return "Wheel";
        default: break;
        }
    }
    if (page == 0x09) return "Button " + std::to_string(usage);
    if (page == 0x07) return "Key " + std::to_string(usage);
    if (page == 0x08) return "LED " + std::to_string(usage);
    char buf[32];
    std::snprintf(buf, sizeof buf, "page 0x%02x usage 0x%02x", page, usage);
    return buf;
}

int32_t extract(const std::vector<uint8_t>& report, const Field& f, uint32_t index)
{
    const uint32_t first = f.bit_offset + index * f.size;
    uint32_t v = 0;
    for (uint32_t b = 0; b < f.size; ++b) {
        const uint32_t bit = first + b;
        if (bit / 8 < report.size() && (report[bit / 8] >> (bit % 8)) & 1) v |= 1u << b;
    }
    if (f.logical_min < 0 && f.size < 32 && (v >> (f.size - 1)) & 1) v |= ~0u << f.size;   // signed
    return static_cast<int32_t>(v);
}

} // namespace hid
