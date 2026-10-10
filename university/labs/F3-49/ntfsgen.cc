// F3-49 ntfsgen.cc: write a small NTFS image from a host directory, for the read-only reader
// to be tested against. It is a test-image generator, not mkntfs: no $LogFile restart pages,
// no $Secure streams, no $AttrDef table, no DOS 8.3 names, no security descriptors.
//
// usage: ntfsgen <out.img> <host dir> <label> [--dirty]
// Files whose name contains "fragmented" are stored in three runs, the second placed before
// the first on disk; files whose name contains "sparse" store all-zero clusters as sparse runs.
#include "ntfs.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

namespace fs = std::filesystem;
constexpr unsigned kCluster = 4096, kRecord = 1024, kIndexBlock = 4096, kFirstUser = 24;
constexpr uint64_t kTime = 133760160000000000ull;   // 2024-11-14 00:00 UTC in 100 ns since 1601

struct Node {
    std::u16string name;
    bool dir = false;
    uint64_t rec = 0, parent = 5;
    Bytes data;
    std::vector<size_t> kids;   // indexes into the node table, for directories
};

struct Volume {
    uint64_t clusters;
    Bytes img;
    std::vector<bool> used;
    uint64_t cursor = 0;
    explicit Volume(uint64_t n) : clusters(n), img(n * kCluster), used(n) {}
    uint64_t alloc(uint64_t n, uint64_t from = ~0ull) {          // first fit, contiguous
        uint64_t at = from == ~0ull ? cursor : from;
        for (;; at++) {
            if (at + n > clusters) throw std::runtime_error("volume full");
            bool free = true;
            for (uint64_t i = 0; i < n && free; i++) free = !used[at + i];
            if (free) break;
        }
        for (uint64_t i = 0; i < n; i++) used[at + i] = true;
        if (from == ~0ull) cursor = at + n;
        return at;
    }
    uint8_t* at(uint64_t lcn) { return &img[lcn * kCluster]; }
};

// One MFT record under construction.
struct Record {
    Bytes b = Bytes(kRecord);
    size_t off = 0x38;
    uint16_t instance = 0;
    explicit Record(uint16_t flags) {        // flags: 1 in use, 2 directory
        std::memcpy(&b[0], "FILE", 4);
        put16(&b[4], 0x30);                  // update sequence array offset
        put16(&b[6], kRecord / kSector + 1); // and its count (number + one per stride)
        put16(&b[0x10], 1);                  // sequence number
        put16(&b[0x12], flags ? 1 : 0);      // hard link count
        put16(&b[0x14], 0x38);               // first attribute
        put16(&b[0x16], flags);
        put32(&b[0x1C], kRecord);            // bytes allocated
    }
    uint8_t* header(uint32_t type, size_t len, bool nonres, const std::u16string& name, size_t nameOfs) {
        if (off + len + 8 > kRecord) throw std::runtime_error("attributes do not fit in one record");
        uint8_t* a = &b[off];
        put32(a, type);
        put32(a + 4, uint32_t(len));
        a[8] = nonres;
        a[9] = uint8_t(name.size());
        put16(a + 0x0A, uint16_t(nameOfs));
        put16(a + 0x0E, instance++);
        for (size_t i = 0; i < name.size(); i++) put16(a + nameOfs + 2 * i, name[i]);
        off += len;
        return a;
    }
    void resident(uint32_t type, const Bytes& v, const std::u16string& name = u"", uint8_t indexed = 0) {
        const size_t vofs = (0x18 + 2 * name.size() + 7) & ~size_t(7);
        uint8_t* a = header(type, (vofs + v.size() + 7) & ~size_t(7), false, name, 0x18);
        put32(a + 0x10, uint32_t(v.size()));
        put16(a + 0x14, uint16_t(vofs));
        a[0x16] = indexed;
        if (!v.empty()) std::memcpy(a + vofs, v.data(), v.size());
    }
    void nonresident(uint32_t type, const std::vector<Run>& runs, uint64_t size,
                     const std::u16string& name = u"", uint16_t flags = 0) {
        const Bytes mp = encodeRuns(runs);
        uint64_t vcns = 0;
        for (const Run& r : runs) vcns += r.len;
        const size_t mofs = (0x40 + 2 * name.size() + 7) & ~size_t(7);
        uint8_t* a = header(type, (mofs + mp.size() + 7) & ~size_t(7), true, name, 0x40);
        put16(a + 0x0C, flags);
        put64(a + 0x10, 0);                           // lowest VCN
        put64(a + 0x18, vcns ? vcns - 1 : 0);         // highest VCN
        put16(a + 0x20, uint16_t(mofs));
        put64(a + 0x28, vcns * kCluster);             // allocated size
        put64(a + 0x30, size);                        // data size
        put64(a + 0x38, size);                        // initialized size
        std::memcpy(a + mofs, mp.data(), mp.size());
    }
    Bytes finish(uint64_t recno) {
        put32(&b[off], kEnd);
        put32(&b[0x18], uint32_t(off + 8));           // bytes in use
        put16(&b[0x28], instance);
        put32(&b[0x2C], uint32_t(recno));
        protect(b.data(), kRecord, 1);
        return b;
    }
};

Bytes stdInfo(uint32_t attrs) {
    Bytes v(72);
    for (int i = 0; i < 4; i++) put64(&v[8 * i], kTime);   // created, modified, MFT changed, accessed
    put32(&v[32], attrs);
    return v;
}
Bytes fileName(uint64_t parent, const std::u16string& name, uint64_t size, uint32_t attrs) {
    Bytes v(0x42 + 2 * name.size());
    put64(&v[0], parent | uint64_t(1) << 48);                // parent reference, sequence 1
    for (int i = 0; i < 4; i++) put64(&v[8 + 8 * i], kTime);
    put64(&v[0x28], (size + kCluster - 1) / kCluster * kCluster);
    put64(&v[0x30], size);
    put32(&v[0x38], attrs);
    v[0x40] = uint8_t(name.size());
    v[0x41] = 1;                                             // namespace: Win32
    for (size_t i = 0; i < name.size(); i++) put16(&v[0x42 + 2 * i], name[i]);
    return v;
}
// An index entry: reference, lengths, flags, the FILE_NAME key, and the child block's VCN.
Bytes indexEntry(uint64_t ref, const Bytes& key, int64_t subnode, bool last) {
    size_t len = (16 + key.size() + 7) & ~size_t(7);
    if (subnode >= 0) len += 8;
    Bytes e(len);
    put64(&e[0], last ? 0 : ref | uint64_t(1) << 48);
    put16(&e[8], uint16_t(len));
    put16(&e[10], uint16_t(key.size()));
    put16(&e[12], uint16_t((subnode >= 0 ? 1 : 0) | (last ? 2 : 0)));
    if (!key.empty()) std::memcpy(&e[16], key.data(), key.size());
    if (subnode >= 0) put64(&e[len - 8], uint64_t(subnode));
    return e;
}
void indexHeader(uint8_t* h, uint32_t entriesOfs, uint32_t used, uint32_t alloc, uint8_t flags) {
    put32(h, entriesOfs);
    put32(h + 4, used);
    put32(h + 8, alloc);
    h[12] = flags;
}

struct Child { std::u16string name; uint64_t rec; Bytes key; };

// The directory's $I30 index: everything in $INDEX_ROOT when it fits, else a two-level B-tree:
// leaves in 4 KiB INDX blocks, separators with subnode pointers in the root.
void addIndex(Record& r, Volume& v, std::vector<Child> kids) {
    std::sort(kids.begin(), kids.end(), [](const Child& a, const Child& b) { return collate(a.name, b.name) < 0; });
    Bytes all;
    for (const Child& c : kids) { Bytes e = indexEntry(c.rec, c.key, -1, false); all.insert(all.end(), e.begin(), e.end()); }
    Bytes endE = indexEntry(0, {}, -1, true);
    all.insert(all.end(), endE.begin(), endE.end());
    auto rootValue = [](const Bytes& entries, bool large) {
        Bytes v(32 + entries.size());
        put32(&v[0], kFileName);              // indexed attribute type
        put32(&v[4], 1);                      // collation rule: file name
        put32(&v[8], kIndexBlock);
        v[12] = kIndexBlock / kCluster;       // clusters per index block
        indexHeader(&v[16], 16, uint32_t(16 + entries.size()), uint32_t(16 + entries.size()), large);
        std::memcpy(&v[32], entries.data(), entries.size());
        return v;
    };
    if (r.off + 0x20 + 32 + all.size() + 8 <= kRecord) { r.resident(kIndexRoot, rootValue(all, false), u"$I30"); return; }

    const size_t entriesAt = 0x40, room = kIndexBlock - entriesAt - 16;   // leave room for the end entry
    std::vector<std::vector<Child>> leaves(1);
    std::vector<Child> separators;
    size_t fill = 0;
    for (const Child& c : kids) {
        const size_t len = indexEntry(c.rec, c.key, -1, false).size();
        if (fill + len > room) { separators.push_back(c); leaves.emplace_back(); fill = 0; continue; }
        leaves.back().push_back(c);
        fill += len;
    }
    const uint64_t lcn = v.alloc(leaves.size());
    for (size_t i = 0; i < leaves.size(); i++) {
        uint8_t* blk = v.at(lcn + i);
        std::memcpy(blk, "INDX", 4);
        put16(blk + 4, 0x28);
        put16(blk + 6, kIndexBlock / kSector + 1);
        put64(blk + 0x10, i);                 // this block's VCN
        size_t p = entriesAt;
        for (const Child& c : leaves[i]) { Bytes e = indexEntry(c.rec, c.key, -1, false); std::memcpy(blk + p, e.data(), e.size()); p += e.size(); }
        std::memcpy(blk + p, endE.data(), endE.size());
        p += endE.size();
        indexHeader(blk + 0x18, uint32_t(entriesAt - 0x18), uint32_t(p - 0x18), kIndexBlock - 0x18, 0);
        protect(blk, kIndexBlock, 1);
    }
    Bytes rootE;
    for (size_t i = 0; i < separators.size(); i++) {
        Bytes e = indexEntry(separators[i].rec, separators[i].key, int64_t(i), false);
        rootE.insert(rootE.end(), e.begin(), e.end());
    }
    Bytes last = indexEntry(0, {}, int64_t(leaves.size() - 1), true);
    rootE.insert(rootE.end(), last.begin(), last.end());
    r.resident(kIndexRoot, rootValue(rootE, true), u"$I30");
    r.nonresident(kIndexAlloc, {{int64_t(lcn), leaves.size()}}, leaves.size() * kIndexBlock, u"$I30");
    Bytes bm(8);
    for (size_t i = 0; i < leaves.size(); i++) bm[i / 8] |= uint8_t(1 << i % 8);
    r.resident(kBitmap, bm, u"$I30");
}

int main(int argc, char** argv) {
    if (argc < 4) { std::cerr << "usage: ntfsgen <out.img> <host dir> <label> [--dirty]\n"; return 2; }
    const bool dirty = argc > 4 && std::string(argv[4]) == "--dirty";
    Volume v(2048);

    // the host tree, depth first, sorted, records from 24 up
    std::vector<Node> nodes(1);
    nodes[0].dir = true;
    nodes[0].rec = 5;
    uint64_t next = kFirstUser;
    std::vector<std::pair<size_t, fs::path>> todo{{0, argv[2]}};
    while (!todo.empty()) {
        auto [idx, dir] = todo.back();
        todo.pop_back();
        std::vector<fs::path> entries;
        for (const auto& e : fs::directory_iterator(dir)) entries.push_back(e.path());
        std::sort(entries.begin(), entries.end());
        for (const fs::path& p : entries) {
            if (fs::is_symlink(p)) { std::cerr << "skipping symbolic link " << p << "\n"; continue; }
            Node n;
            n.name = fromUtf8(p.filename().string());
            n.dir = fs::is_directory(p);
            n.rec = next++;
            n.parent = nodes[idx].rec;
            if (!n.dir) {
                std::ifstream in(p, std::ios::binary);
                n.data.assign(std::istreambuf_iterator<char>(in), {});
            }
            nodes.push_back(n);
            nodes[idx].kids.push_back(nodes.size() - 1);
            if (n.dir) todo.push_back({nodes.size() - 1, p});
        }
    }
    const uint64_t nrec = next;

    // fixed system areas
    v.alloc(2, 0);                                           // $Boot: clusters 0-1
    const uint64_t mft1 = v.alloc(kFirstUser * kRecord / kCluster, 4);       // records 0-23
    const uint64_t mft2len = (nrec - kFirstUser + 3) / 4;
    const uint64_t mft2 = v.alloc(mft2len, v.clusters * 3 / 4);              // the rest, far away
    const uint64_t mirr = v.alloc(1, v.clusters / 2);
    v.cursor = mft1 + kFirstUser * kRecord / kCluster;
    const uint64_t logLcn = v.alloc(16), upLcn = v.alloc(32), bmLcn = v.alloc(1);
    for (unsigned c = 0; c < 65536; c++) put16(v.at(upLcn) + 2 * c, upcase(char16_t(c)));

    // file data
    std::map<uint64_t, std::vector<Run>> runsOf;
    for (Node& n : nodes) {
        if (n.dir || n.data.size() <= 700) continue;
        const uint64_t nc = (n.data.size() + kCluster - 1) / kCluster;
        const std::string name = toUtf8(n.name);
        std::vector<Run> runs;
        if (name.find("fragmented") != std::string::npos && nc >= 3) {
            const uint64_t a = nc / 3, b = nc / 3, c = nc - a - b;
            const uint64_t second = v.alloc(b), first = v.alloc(a), third = v.alloc(c);
            runs = {{int64_t(first), a}, {int64_t(second), b}, {int64_t(third), c}};
        } else if (name.find("sparse") != std::string::npos) {
            for (uint64_t i = 0; i < nc; i++) {
                const size_t lo = i * kCluster, hi = std::min(n.data.size(), lo + kCluster);
                const bool zero = std::all_of(n.data.begin() + lo, n.data.begin() + hi, [](uint8_t x) { return x == 0; });
                const int64_t lcn = zero ? -1 : int64_t(v.alloc(1));
                if (!runs.empty() && ((zero && runs.back().lcn < 0) ||
                                      (!zero && runs.back().lcn >= 0 && runs.back().lcn + int64_t(runs.back().len) == lcn)))
                    runs.back().len++;
                else
                    runs.push_back({lcn, 1});
            }
        } else {
            runs = {{int64_t(v.alloc(nc)), nc}};
        }
        uint64_t vcn = 0;
        for (const Run& r : runs) {
            for (uint64_t i = 0; i < r.len; i++, vcn++) {
                const size_t lo = vcn * kCluster, hi = std::min(n.data.size(), lo + kCluster);
                if (r.lcn >= 0 && lo < hi) std::memcpy(v.at(r.lcn + i), &n.data[lo], hi - lo);
            }
        }
        runsOf[n.rec] = runs;
    }

    // directory children (their keys are copies of their $FILE_NAME values)
    std::map<uint64_t, std::vector<Child>> childrenOf;
    std::map<uint64_t, Bytes> fnOf;
    for (const Node& n : nodes) {
        if (n.rec == 5) continue;
        fnOf[n.rec] = fileName(n.parent, n.name, n.data.size(), n.dir ? 0x10000000 : 0x20);
        childrenOf[n.parent].push_back({n.name, n.rec, fnOf[n.rec]});
    }
    const char16_t* sysNames[] = {u"$MFT", u"$MFTMirr", u"$LogFile", u"$Volume", u"$AttrDef", u".",
                                  u"$Bitmap", u"$Boot", u"$BadClus", u"$Secure", u"$UpCase", u"$Extend"};
    for (uint64_t r = 0; r < 12; r++) {
        fnOf[r] = fileName(5, sysNames[r], 0, r == 5 || r == 11 ? 0x10000006 : 0x06);
        childrenOf[5].push_back({sysNames[r], r, fnOf[r]});
    }

    // the records
    std::vector<Bytes> recs(nrec);
    for (uint64_t r = 0; r < nrec; r++) {
        const Node* n = nullptr;
        for (const Node& x : nodes) if (x.rec == r) n = &x;
        const bool dir = r == 5 || r == 11 || (n && n->dir);
        if (r >= 12 && r < kFirstUser) { recs[r] = Record(0).finish(r); continue; }   // reserved, not in use
        Record rec(dir ? 3 : 1);
        rec.resident(kStdInfo, stdInfo(r < kFirstUser ? 0x06 : n->dir ? 0 : 0x20));
        rec.resident(kFileName, fnOf[r], u"", 1);
        switch (r) {
        case 0: {
            rec.nonresident(kData, {{int64_t(mft1), kFirstUser * kRecord / kCluster}, {int64_t(mft2), mft2len}}, nrec * kRecord);
            Bytes bm((nrec + 63) / 64 * 8);
            for (uint64_t i = 0; i < nrec; i++) if (i < 12 || i >= kFirstUser) bm[i / 8] |= uint8_t(1 << i % 8);
            rec.resident(kBitmap, bm);
            break;
        }
        case 1: rec.nonresident(kData, {{int64_t(mirr), 1}}, 4 * kRecord); break;
        case 2: rec.nonresident(kData, {{int64_t(logLcn), 16}}, 16 * kCluster); break;
        case 3: {
            const std::u16string label = fromUtf8(argv[3]);
            Bytes name(2 * label.size()), info(12);
            for (size_t i = 0; i < label.size(); i++) put16(&name[2 * i], label[i]);
            info[8] = 3;                       // NTFS 3.1
            info[9] = 1;
            put16(&info[10], dirty ? 1 : 0);   // volume flags: 1 = dirty
            rec.resident(kVolName, name);
            rec.resident(kVolInfo, info);
            rec.resident(kData, {});
            break;
        }
        case 6: rec.nonresident(kData, {{int64_t(bmLcn), 1}}, v.clusters / 8); break;
        case 7: rec.nonresident(kData, {{0, 2}}, 2 * kCluster); break;
        case 10: rec.nonresident(kData, {{int64_t(upLcn), 32}}, 65536 * 2); break;
        case 5: case 11: break;
        default:
            if (!dir) {
                if (runsOf.count(r)) {
                    const bool sparse = toUtf8(n->name).find("sparse") != std::string::npos;
                    rec.nonresident(kData, runsOf[r], n->data.size(), u"", sparse ? 0x8000 : 0);
                } else {
                    rec.resident(kData, n ? n->data : Bytes{});
                }
            }
        }
        if (dir) addIndex(rec, v, childrenOf[r]);
        recs[r] = rec.finish(r);
    }

    // the cluster bitmap, the MFT in its two runs, the mirror, the boot sector and its backup
    for (uint64_t c = 0; c < v.clusters; c++) if (v.used[c]) v.at(bmLcn)[c / 8] |= uint8_t(1 << c % 8);
    for (uint64_t r = 0; r < nrec; r++) {
        const uint64_t byte = r * kRecord;
        const uint64_t firstLen = kFirstUser * kRecord;
        uint8_t* dst = byte < firstLen ? v.at(mft1) + byte : v.at(mft2) + (byte - firstLen);
        std::memcpy(dst, recs[r].data(), kRecord);
        if (r < 4) std::memcpy(v.at(mirr) + byte, recs[r].data(), kRecord);
    }
    uint8_t* bs = v.at(0);
    const uint8_t jump[3] = {0xEB, 0x52, 0x90};
    std::memcpy(bs, jump, 3);
    std::memcpy(bs + 3, "NTFS    ", 8);
    put16(bs + 0x0B, kSector);
    bs[0x0D] = kCluster / kSector;
    bs[0x15] = 0xF8;
    put16(bs + 0x18, 63);
    put16(bs + 0x1A, 255);
    bs[0x24] = 0x80;
    bs[0x26] = 0x80;
    put64(bs + 0x28, v.clusters * (kCluster / kSector) - 1);   // the last sector holds the backup
    put64(bs + 0x30, mft1);
    put64(bs + 0x38, mirr);
    bs[0x40] = 0xF6;                                         // -10: records are 2^10 bytes
    bs[0x44] = kIndexBlock / kCluster;
    put64(bs + 0x48, 0x0401F3490401F349ull);                 // volume serial number
    bs[0x1FE] = 0x55;
    bs[0x1FF] = 0xAA;
    std::memcpy(&v.img[v.img.size() - kSector], bs, kSector);

    std::ofstream(argv[1], std::ios::binary).write(reinterpret_cast<const char*>(v.img.data()), std::streamsize(v.img.size()));
    uint64_t usedC = 0;
    for (bool b : v.used) usedC += b;
    std::printf("%s: %llu clusters of %u bytes, %llu in use; MFT at %llu (+%llu records at %llu), mirror at %llu, %llu records\n",
                argv[1], (unsigned long long)v.clusters, kCluster, (unsigned long long)usedC, (unsigned long long)mft1,
                (unsigned long long)(nrec - kFirstUser), (unsigned long long)mft2, (unsigned long long)mirr, (unsigned long long)nrec);
    return 0;
}
