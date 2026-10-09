// stream.h - F3-44: reading a recorded write stream. showStream() prints it; crashImage()
// builds the disk as a power cut could leave it: every write before the last FLUSH that was
// issued lands; each write after that FLUSH lands or not (a coin toss per write).
#pragma once
#include "blockdev.h"
#include <algorithm>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace os401 {

struct Rec
{
    char type;                 // 'W' write, 'F' flush, 'P' promise
    std::uint32_t blk;
    std::size_t at;            // where the block's bytes start in the stream
    std::string text;
};
constexpr std::uint32_t kStreamBs = 1024;

inline std::vector<Rec> parseStream(const Bytes& log)
{
    std::vector<Rec> recs;
    for (std::size_t i = 0; i < log.size();) {
        const char t = static_cast<char>(log[i]);
        if (t == 'W') {
            recs.push_back({t, get32(&log[i + 1]), i + 5, ""});
            i += 5 + kStreamBs;
        } else if (t == 'F') {
            recs.push_back({t, 0, 0, ""});
            i += 1;
        } else if (t == 'P') {
            const std::uint32_t n = get16(&log[i + 1]);
            const auto from = log.begin() + static_cast<long>(i) + 3;
            recs.push_back({t, 0, 0, std::string(from, from + n)});
            i += 3 + n;
        } else {
            throw std::runtime_error("bad write stream");
        }
    }
    return recs;
}

inline int showStream(const std::string& stream, std::size_t first, std::size_t count)
{
    const std::vector<Rec> recs = parseStream(loadFile(stream));
    for (std::size_t i = first; i < recs.size() && i < first + count; ++i) {
        if (recs[i].type == 'W') std::printf("%6zu  write block %u\n", i, recs[i].blk);
        if (recs[i].type == 'F') std::printf("%6zu  FLUSH\n", i);
        if (recs[i].type == 'P') std::printf("%6zu  fsync returned: %s\n", i, recs[i].text.c_str());
    }
    return 0;
}

// cutAt < 0: the cut point is random too; otherwise exactly cutAt records were issued.
inline int crashImage(const std::string& base, const std::string& stream, unsigned seed, const std::string& out,
                      long cutAt = -1)
{
    Bytes img = loadFile(base);
    const Bytes log = loadFile(stream);
    const std::vector<Rec> recs = parseStream(log);
    constexpr std::uint32_t bs = kStreamBs;
    std::mt19937 rng(seed);
    std::size_t cut = rng() % (recs.size() + 1);                   // records issued before the cut
    if (cutAt >= 0) cut = std::min(static_cast<std::size_t>(cutAt), recs.size());
    std::size_t lastFlush = 0;
    for (std::size_t i = 0; i < cut; ++i) {
        if (recs[i].type == 'F') lastFlush = i + 1;
    }
    std::size_t durable = 0, cached = 0, landed = 0;
    for (std::size_t i = 0; i < cut; ++i) {
        if (recs[i].type != 'W') continue;
        const bool lands = i < lastFlush || (++cached, rng() % 2 == 0);
        if (!lands) continue;
        (i < lastFlush ? durable : landed) += 1;
        std::copy(log.begin() + static_cast<long>(recs[i].at), log.begin() + static_cast<long>(recs[i].at + bs),
                  img.begin() + static_cast<long>(recs[i].blk) * bs);
    }
    saveFile(out, img);
    std::printf("cut after record %zu of %zu; durable writes %zu; cached writes %zu, of which landed %zu\n",
                cut, recs.size(), durable, cached, landed);
    for (std::size_t i = 0; i < cut; ++i) {
        if (recs[i].type == 'P') std::printf("promise\t%s\n", recs[i].text.c_str());
    }
    return 0;
}

} // namespace os401
