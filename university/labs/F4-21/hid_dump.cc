// hid_dump.cc - parse each report descriptor found by hid_extract.py (lines on standard input:
// "offset usage-0xNN length hex") and print its collections, its input fields and the length of
// each input report. Self-check: every input report must be a whole number of bytes.
#include "hid_parse.h"
#include <cstdio>
#include <iostream>
#include <sstream>

int main()
{
    std::string line;
    int bad = 0, n = 0;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream in(line);
        std::string offset, usage, len, hex;
        in >> offset >> usage >> len >> hex;
        std::vector<uint8_t> d;
        for (size_t i = 0; i + 1 < hex.size(); i += 2) d.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
        const hid::Parsed p = hid::parse(d);
        ++n;
        std::printf("== descriptor at file offset %s, %zu bytes ==\n", offset.c_str(), d.size());
        const int before = bad;
        for (const auto& c : p.collections) std::printf("  collection %s\n", c.c_str());
        for (const auto& f : p.fields) {
            std::printf("  input id %u bits %3u..%3u  %u x %2u bits  %-10s %-26s logical %d..%d\n",
                        f.report_id, f.bit_offset, f.bit_offset + f.size * f.count - 1, f.count, f.size,
                        (f.flags & 1) ? "constant" : ((f.flags & 4) ? "relative" : "absolute"),
                        (f.flags & 1) ? "(padding)" :
                        (f.usage == f.usage_max ? hid::usage_name(f.usage_page, f.usage)
                         : hid::usage_name(f.usage_page, f.usage) + " .. " + hid::usage_name(f.usage_page, f.usage_max)).c_str(),
                        f.logical_min, f.logical_max);
        }
        for (const auto& r : p.reports) {
            const bool whole = r.bits % 8 == 0;
            std::printf("  input report id %u: %u bits = %u bytes%s%s\n", r.id, r.bits, r.bits / 8,
                        r.id ? " + 1 report-ID byte" : "", whole ? "" : "  NOT A WHOLE NUMBER OF BYTES");
            if (!whole) ++bad;
        }
        for (const auto& w : p.warnings) std::printf("  warning: %s\n", w.c_str());
        if (!p.error.empty()) { std::printf("  ERROR: %s\n", p.error.c_str()); ++bad; }
        if (bad != before) {                     // a problem: show every item, to find the cause
            std::printf("  items (offset: bytes, type, tag):\n");
            for (const auto& it : p.items) std::printf("    %s\n", it.c_str());
        }
    }
    std::printf("%d descriptors, %d problems\n", n, bad);
    return bad ? 1 : 0;
}
