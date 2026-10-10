// annotate.cc - turn a raw access trace of one device into an observer's first description:
//   1. the sequence, with read-only scans and polling loops folded into one line each;
//   2. per register: how often it is read and written, and what the first read after a write
//      returns (the inverse, the same value, something else); changes with no write in between;
//   3. cross effects: a write to one offset followed by a changed value at another offset.
// Input: the output of trace_filter.py on standard input. The program states only what the
// trace shows; turning it into a specification is the observer's job (spec_observed.txt).
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Access { int seq; bool write; uint32_t off; uint32_t value; };

int main()
{
    std::vector<Access> t;
    std::string line;
    while (std::getline(std::cin, line)) {
        Access a{};
        char rw = 0;
        unsigned off = 0, val = 0;
        if (std::sscanf(line.c_str(), "#%d %c 0x%x 0x%x", &a.seq, &rw, &off, &val) == 4) {
            a.write = rw == 'W';
            a.off = off;
            a.value = val;
            t.push_back(a);
        }
    }
    std::cout << "== 1. sequence (" << t.size() << " accesses) ==\n";
    for (std::size_t i = 0; i < t.size();) {
        std::size_t j = i;   // a scan: consecutive reads at offsets 0, 4, 8, ...
        while (j < t.size() && !t[j].write && t[j].off == 4 * (j - i)) ++j;
        if (j - i >= 16) {
            std::printf("#%d-#%d  read-only scan of offsets 0x00-0x%02x (%zu reads)\n", t[i].seq, t[j - 1].seq,
                        t[j - 1].off, j - i);
            i = j;
            continue;
        }
        j = i;               // a poll: the same offset read again and again
        while (j < t.size() && !t[j].write && t[j].off == t[i].off) ++j;
        if (j - i >= 2) {
            std::printf("#%d-#%d  poll 0x%02x x%zu: 0x%08x ... 0x%08x\n", t[i].seq, t[j - 1].seq, t[i].off, j - i,
                        t[i].value, t[j - 1].value);
            i = j;
            continue;
        }
        std::printf("#%d      %s 0x%02x = 0x%08x\n", t[i].seq, t[i].write ? "write" : "read ", t[i].off, t[i].value);
        ++i;
    }

    std::cout << "== 2. per register ==\n";
    std::map<uint32_t, std::vector<const Access*>> by_off;
    for (const auto& a : t) by_off[a.off].push_back(&a);
    for (const auto& [off, list] : by_off) {
        int r = 0, w = 0, inverse = 0, echo = 0, other = 0, changed_alone = 0;
        bool have_w = false, have_r = false, wrote_since_read = false;
        uint32_t last_w = 0, last_r = 0;
        std::set<uint32_t> reads;
        for (const Access* a : list) {
            if (a->write) { ++w; last_w = a->value; have_w = true; wrote_since_read = true; continue; }
            ++r;
            reads.insert(a->value);
            if (wrote_since_read) {          // the first read after a write
                if (a->value == static_cast<uint32_t>(~last_w)) ++inverse;
                else if (a->value == last_w) ++echo;
                else ++other;
            } else if (have_r && a->value != last_r) {
                ++changed_alone;
            }
            last_r = a->value;
            have_r = true;
            wrote_since_read = false;
        }
        if (w == 0 && reads.size() == 1 && *reads.begin() == 0xFFFFFFFFu) continue;   // reported below
        std::printf("0x%02x  reads %d, writes %d", off, r, w);
        if (reads.size() == 1) std::printf(", always reads 0x%08x", *reads.begin());
        else std::printf(", %zu distinct values read", reads.size());
        if (inverse) std::printf("; read after write = inverse of the value written (%dx)", inverse);
        if (echo) std::printf("; read after write = the value written (%dx)", echo);
        if (other) std::printf("; read after write = a different value (%dx)", other);
        if (changed_alone) std::printf("; changed between two reads with no write in between (%dx)", changed_alone);
        if (have_w && !have_r) std::printf("; never read");
        std::printf("\n");
    }
    std::string ones;
    for (const auto& [off, list] : by_off) {
        bool all = !list.empty();
        for (const Access* a : list) all = all && !a->write && a->value == 0xFFFFFFFFu;
        if (all) { char buf[8]; std::snprintf(buf, sizeof buf, " 0x%02x", off); ones += buf; }
    }
    std::cout << "offsets that only ever read 0xffffffff:" << ones << '\n';

    std::cout << "== 3. cross effects (a write, then another offset reads differently than before) ==\n";
    std::map<uint32_t, uint32_t> last_read;
    for (std::size_t i = 0; i < t.size(); ++i) {
        if (!t[i].write) { last_read[t[i].off] = t[i].value; continue; }
        std::set<uint32_t> seen;
        for (std::size_t j = i + 1; j < t.size() && !t[j].write; ++j) {
            const uint32_t o = t[j].off;
            if (o == t[i].off || seen.count(o) || !last_read.count(o)) continue;
            seen.insert(o);
            if (t[j].value != last_read[o])
                std::printf("#%d write 0x%02x = 0x%08x  ->  #%d 0x%02x now reads 0x%08x (was 0x%08x)\n", t[i].seq, t[i].off,
                            t[i].value, t[j].seq, o, t[j].value, last_read[o]);
        }
    }
    return 0;
}
