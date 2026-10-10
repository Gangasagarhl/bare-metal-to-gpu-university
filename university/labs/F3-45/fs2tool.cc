// fs2tool.cc - F3-45: the ext2 writer of F3-44 on top of a JBD2 journal (milestone FS2).
//   fs2tool workload <img> <stream> <seed> <ops> journal|nobarrier <expectdir>
//       mount (replaying if needed), run the F3-44 workload with one transaction per
//       operation, unmount; "nobarrier" leaves out the flush before the commit block
//   fs2tool recover <img>        replay the journal of a crashed image (our recovery code)
//   fs2tool crash <base.img> <stream> <seed> <out.img> [cut]
//       as in F3-44; with [cut], the power fails after exactly that many records
//   fs2tool commits <stream>     the cut points right after each commit-block write
//   fs2tool showj <img> <stream> <first> <count>
//       like show, but names each write: journal superblock, descriptor, log copy, commit,
//       or in-place write (the journal's location is read from <img>)
//   fs2tool show <stream> <first> <count>                  as in F3-44
#include "journal.h"
#include "../F3-44/stream.h"
#include "../F3-44/workload.h"
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kBs = 1024;

std::vector<std::uint32_t> mapJournal(os401::BlockDev& dev)
{
    os401::DirectStore probe(dev, true);                    // reads only
    os401::Ext2 fs(probe, kBs, false, 0x2 | os401::kRecoverFlag);
    return os401::journalMap(fs, kBs);
}

int showJournal(const std::string& img, const std::string& stream, std::size_t first, std::size_t count)
{
    os401::BlockDev dev(img, kBs, "");
    const std::vector<std::uint32_t> jmap = mapJournal(dev);
    std::map<std::uint32_t, std::uint32_t> where;          // device block -> journal block
    for (std::uint32_t i = 0; i < jmap.size(); ++i) where[jmap[i]] = i;
    const os401::Bytes log = os401::loadFile(stream);
    const std::vector<os401::Rec> recs = os401::parseStream(log);
    std::vector<std::uint32_t> targets;                    // tags of the last descriptor
    std::size_t nextTarget = 0;
    for (std::size_t i = 0; i < recs.size() && i < first + count; ++i) {
        std::string what;
        if (recs[i].type == 'F') what = "FLUSH";
        if (recs[i].type == 'P') what = "fsync returned: " + recs[i].text;
        if (recs[i].type == 'W') {
            const std::uint8_t* b = &log[recs[i].at];
            auto j = where.find(recs[i].blk);
            const bool hdr = os401::be32(b) == os401::kJMagic;
            char buf[160];
            if (j == where.end()) {
                std::snprintf(buf, sizeof buf, "write block %u in place", recs[i].blk);
            } else if (j->second == 0) {
                std::snprintf(buf, sizeof buf, "journal block 0: journal superblock, s_sequence %u, s_start %u",
                              os401::be32(b + 24), os401::be32(b + 28));
            } else if (hdr && os401::be32(b + 4) == os401::kDescriptor) {
                targets.clear();
                nextTarget = 0;
                for (std::uint32_t off = 12;; ) {
                    const std::uint32_t flags = static_cast<std::uint32_t>(b[off + 6] << 8 | b[off + 7]);
                    targets.push_back(os401::be32(b + off));
                    off += 8 + ((flags & os401::kSameUuid) ? 0 : 16);
                    if (flags & os401::kLastTag) break;
                }
                std::snprintf(buf, sizeof buf, "journal block %u: DESCRIPTOR, sequence %u, %zu tags",
                              j->second, os401::be32(b + 8), targets.size());
            } else if (hdr && os401::be32(b + 4) == os401::kCommit) {
                std::snprintf(buf, sizeof buf, "journal block %u: COMMIT, sequence %u", j->second, os401::be32(b + 8));
            } else {
                std::snprintf(buf, sizeof buf, "journal block %u: log copy of block %u", j->second,
                              nextTarget < targets.size() ? targets[nextTarget] : 0);
                ++nextTarget;
            }
            what = buf;
        }
        if (i >= first) std::printf("%6zu  %s\n", i, what.c_str());
    }
    return 0;
}

void report(const char* what, const os401::ReplayReport& r)
{
    std::printf("%s: %d transaction(s) replayed, %d block(s) written, %d revoked; next sequence %u\n",
                what, r.transactions, r.blocks, r.revoked, r.nextSequence);
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const std::vector<std::string> a(argv + 1, argv + argc);
        if (a.size() == 7 && a[0] == "workload") {
            os401::BlockDev dev(a[1], kBs, a[2]);
            const std::vector<std::uint32_t> jmap = mapJournal(dev);
            os401::JournalStore js(dev, jmap, a[5] != "nobarrier");
            report("mount", js.mount());
            os401::Ext2 fs(js, kBs, false, 0x2 | os401::kRecoverFlag);
            const int rc = os401::runWorkload(fs, dev, static_cast<unsigned>(std::stoul(a[3])), std::stoi(a[4]), true,
                                              a[6], a[5] == "nobarrier" ? "journal WITHOUT the flush before commit"
                                                                        : "journal, ordered mode",
                                              [&js] { js.commit(); });
            js.unmount();
            std::printf("  transactions committed %lu, metadata blocks logged %lu\n", js.commits(), js.logged());
            dev.save(a[1]);
            return rc;
        }
        if (a.size() == 2 && a[0] == "recover") {
            os401::BlockDev dev(a[1], kBs, "");
            report("recover", os401::recover(dev, mapJournal(dev)));
            dev.save(a[1]);
            return 0;
        }
        if (a.size() == 4 && a[0] == "show")
            return os401::showStream(a[1], std::stoul(a[2]), std::stoul(a[3]));
        if ((a.size() == 5 || a.size() == 6) && a[0] == "crash")
            return os401::crashImage(a[1], a[2], static_cast<unsigned>(std::stoul(a[3])), a[4],
                                     a.size() == 6 ? std::stol(a[5]) : -1);
        if (a.size() == 5 && a[0] == "showj")
            return showJournal(a[1], a[2], std::stoul(a[3]), std::stoul(a[4]));
        if (a.size() == 2 && a[0] == "commits") {
            const os401::Bytes log = os401::loadFile(a[1]);
            const std::vector<os401::Rec> recs = os401::parseStream(log);
            for (std::size_t i = 0; i < recs.size(); ++i) {
                const std::uint8_t* b = &log[recs[i].at];
                if (recs[i].type == 'W' && os401::be32(b) == os401::kJMagic && os401::be32(b + 4) == os401::kCommit)
                    std::printf("%zu\n", i + 1);
            }
            return 0;
        }
        std::fprintf(stderr, "usage: see the comment at the top of fs2tool.cc\n");
        return 2;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fs2tool: %s\n", e.what());
        return 1;
    }
}
