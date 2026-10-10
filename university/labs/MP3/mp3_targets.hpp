// MP3: the targets file agreed at gate R1, read by targets_lock.cpp and mp3_report.cpp.
// One record per line, '#' starts a comment. Kinds used by MP3:
//   gpu|driver|toolkit|clocks <text>          the machine the targets are for
//   reference <kernel> <library routine>     what "100 %" means for that kernel
//   target <kernel> <size> <precision> <pct> the pre-agreed percentage of the reference
//   tolerance <precision> <rule>             the correctness rule for that precision
//   author <name>   signed <gate> <reviewer> <date>
// The fingerprint covers every line that defines the agreement (not comments, not signatures),
// so any later change to a target, a reference or the machine changes it.
#pragma once
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

struct Line
{
    std::string kind;
    std::vector<std::string> words;   // the words after the kind
};

inline std::vector<Line> parseRecords(std::istream& in)
{
    std::vector<Line> out;
    std::string text;
    while (std::getline(in, text)) {
        const std::size_t hash = text.find('#');
        if (hash != std::string::npos) {
            text.erase(hash);
        }
        std::istringstream ws(text);
        Line l;
        if (!(ws >> l.kind)) {
            continue;                                  // blank or comment-only line
        }
        for (std::string w; ws >> w;) {
            l.words.push_back(w);
        }
        out.push_back(l);
    }
    return out;
}

inline bool definesAgreement(const Line& l)
{
    return l.kind == "gpu" || l.kind == "driver" || l.kind == "toolkit" || l.kind == "clocks" ||
           l.kind == "reference" || l.kind == "target" || l.kind == "tolerance";
}

// FNV-1a, 64 bit: not a security hash, only a short value that changes when the text changes.
inline std::uint64_t fingerprint(const std::vector<Line>& lines)
{
    std::uint64_t h = 1469598103934665603ULL;
    auto eat = [&h](const std::string& s) {
        for (unsigned char c : s) {
            h ^= c;
            h *= 1099511628211ULL;
        }
        h ^= 0x1F;                                     // word separator
        h *= 1099511628211ULL;
    };
    for (const Line& l : lines) {
        if (definesAgreement(l)) {
            eat(l.kind);
            for (const std::string& w : l.words) {
                eat(w);
            }
        }
    }
    return h;
}

inline std::string hex64(std::uint64_t v)
{
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(v));
    return buf;
}

inline std::string joined(const std::vector<std::string>& w, std::size_t from = 0)
{
    std::string s;
    for (std::size_t i = from; i < w.size(); ++i) {
        s += (i > from ? " " : "") + w[i];
    }
    return s;
}
