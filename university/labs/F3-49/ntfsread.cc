// F3-49 ntfsread.cc: a read-only NTFS reader. It opens the image for reading only and has no
// code path that writes; a volume marked dirty is read with a warning, never "fixed".
//
// usage: ntfsread info <img>            boot sector, MFT location, volume name, version, flags
//        ntfsread record <img> <n>      one MFT record: header, fixups, attributes, run lists
//        ntfsread ls <img> [-a]         every file and directory (-a: with the system files)
//        ntfsread find <img> <path>     B-tree lookup, showing the index nodes visited
//        ntfsread cat <img> <path>      a file's unnamed $DATA to standard output
//        ntfsread check <img>           fixups of every record; MFT bitmap; cluster bitmap; mirror
#include "ntfs.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>

struct Attr {
    uint32_t type = 0;
    std::u16string name;
    bool nonres = false;
    uint16_t flags = 0;
    Bytes value;                  // resident
    std::vector<Run> runs;        // non-resident
    uint64_t size = 0, init = 0;
};
struct Entry { std::u16string name; uint64_t ref; uint8_t nameSpace; };

uint32_t crc32(const Bytes& d) {
    uint32_t c = 0xFFFFFFFF;
    for (uint8_t b : d) { c ^= b; for (int k = 0; k < 8; k++) c = c >> 1 ^ (0xEDB88320 & (0 - (c & 1))); }
    return ~c;
}

class Ntfs {
public:
    unsigned sector = 0, cluster = 0, recSize = 0, idxSize = 0;
    uint64_t sectors = 0, mftLcn = 0, mirrLcn = 0, serial = 0;
    std::vector<Run> mftRuns;
    uint64_t mftBytes = 0;
    bool usedMirror = false;
    Bytes rec0;                   // record 0 as used (from the $MFT or from the mirror)

    explicit Ntfs(const char* path) : f_(path, std::ios::binary) {
        if (!f_) throw std::runtime_error("cannot open image");
        Bytes bs = read(0, 512);
        if (std::memcmp(&bs[3], "NTFS    ", 8) || bs[510] != 0x55 || bs[511] != 0xAA) throw std::runtime_error("not an NTFS boot sector");
        sector = le16(&bs[0x0B]);
        cluster = sector * bs[0x0D];
        sectors = le64(&bs[0x28]);
        mftLcn = le64(&bs[0x30]);
        mirrLcn = le64(&bs[0x38]);
        serial = le64(&bs[0x48]);
        recSize = sizeField(int8_t(bs[0x40]));
        idxSize = sizeField(int8_t(bs[0x44]));
        if (sector < 256 || cluster == 0 || recSize < 512 || recSize > 4096) throw std::runtime_error("implausible geometry");
        // Record 0 describes the MFT itself; it is read from where the boot sector points, and
        // from the mirror if its fixups do not check out.
        Bytes r0 = read(mftLcn * cluster, recSize);
        if (std::memcmp(r0.data(), "FILE", 4) || unprotect(r0.data(), recSize)) {
            r0 = read(mirrLcn * cluster, recSize);
            usedMirror = true;
            if (std::memcmp(r0.data(), "FILE", 4) || unprotect(r0.data(), recSize)) throw std::runtime_error("$MFT and $MFTMirr record 0 both unreadable");
        }
        rec0 = r0;
        for (const Attr& a : attrs(r0))
            if (a.type == kData && a.name.empty()) { mftRuns = a.runs; mftBytes = a.size; }
        if (mftRuns.empty()) throw std::runtime_error("$MFT has no unnamed $DATA");
    }
    uint64_t records() const { return mftBytes / recSize; }

    // Read MFT record r; on failure, explain why in err and return an empty buffer.
    Bytes record(uint64_t r, std::string* err = nullptr) {
        Bytes b = readRuns(mftRuns, r * recSize, recSize);
        std::string why;
        if (std::memcmp(b.data(), "FILE", 4)) why = "no FILE signature";
        else if (unsigned s = unprotect(b.data(), recSize))
            why = "update sequence mismatch in stride " + std::to_string(s) + " (torn or damaged write)";
        else if (le32(&b[0x18]) > recSize || le16(&b[0x14]) >= recSize) why = "header out of range";
        if (why.empty()) return b;
        if (err) *err = why;
        else throw std::runtime_error("MFT record " + std::to_string(r) + ": " + why);
        return {};
    }

    static std::vector<Attr> attrs(const Bytes& rec) {
        std::vector<Attr> out;
        size_t off = le16(&rec[0x14]);
        const size_t end = le32(&rec[0x18]);
        while (off + 8 <= end) {
            const uint8_t* a = &rec[off];
            if (le32(a) == kEnd) break;
            const uint32_t len = le32(a + 4);
            if (len < 0x18 || off + len > end) throw std::runtime_error("attribute runs past the record");
            Attr x;
            x.type = le32(a);
            x.nonres = a[8];
            x.flags = le16(a + 0x0C);
            for (unsigned i = 0; i < a[9]; i++) x.name += char16_t(le16(a + le16(a + 0x0A) + 2 * i));
            if (!x.nonres) {
                const uint32_t vl = le32(a + 0x10), vo = le16(a + 0x14);
                if (vo + vl > len) throw std::runtime_error("resident value runs past its attribute");
                x.value.assign(a + vo, a + vo + vl);
                x.size = x.init = vl;
            } else {
                x.runs = decodeRuns(a + le16(a + 0x20), a + len);
                x.size = le64(a + 0x30);
                x.init = le64(a + 0x38);
            }
            out.push_back(x);
            off += len;
        }
        return out;
    }

    Bytes data(const Attr& a) {
        if (a.flags & 0x40FF) throw std::runtime_error("compressed or encrypted $DATA: not supported, refused rather than guessed");
        if (!a.nonres) return a.value;
        Bytes d = readRuns(a.runs, 0, a.size);
        for (uint64_t i = a.init; i < d.size(); i++) d[i] = 0;    // past "initialized": zeros
        return d;
    }

    // Directory: an in-order walk of the $I30 B-tree (root in the record, nodes in INDX blocks).
    std::vector<Entry> list(uint64_t dirRec) {
        Ctx c = ctx(dirRec);
        std::vector<Entry> out;
        walk(c, &c.root.value[16], c.root.value.size() - 16, out, u"", u"");
        return out;
    }
    // Lookup by name, descending one node per level; trace says which nodes were read.
    uint64_t find(uint64_t dirRec, const std::u16string& name, std::string& trace) {
        Ctx c = ctx(dirRec);
        uint64_t found = ~0ull;
        Bytes keep;
        const uint8_t* hdr = &c.root.value[16];
        size_t avail = c.root.value.size() - 16;
        trace += "record " + std::to_string(dirRec) + " $INDEX_ROOT";
        for (int depth = 0; depth < 16; depth++) {
            int64_t down = -1;
            for (const uint8_t* p = hdr + le32(hdr); p + 16 <= hdr + std::min<size_t>(le32(hdr + 4), avail);) {
                const uint16_t len = le16(p + 8), fl = le16(p + 12);
                if (len < 16) throw std::runtime_error("index entry length 0");
                const int cmp = fl & 2 ? -1 : collate(name, keyName(p + 16));
                if (cmp == 0) { found = le64(p) & kRefMask; break; }
                if (cmp < 0) { if (fl & 1) down = int64_t(le64(p + len - 8)); break; }
                p += len;
            }
            if (found != ~0ull || down < 0) break;
            keep = indexBlock(c, uint64_t(down));
            hdr = &keep[0x18];
            avail = idxSize - 0x18;
            trace += " -> INDX VCN " + std::to_string(down);
        }
        return found;
    }
    uint64_t resolve(const std::string& path, std::string* trace = nullptr) {
        uint64_t r = 5;
        size_t i = 0;
        std::string t;
        while (i < path.size()) {
            while (i < path.size() && path[i] == '/') i++;
            if (i == path.size()) break;
            const size_t j = path.find('/', i);
            const std::string part = path.substr(i, j == std::string::npos ? std::string::npos : j - i);
            std::string step;
            r = find(r, fromUtf8(part), step);
            t += "\"" + part + "\": " + step + (r == ~0ull ? " -> not found\n" : " -> record " + std::to_string(r) + "\n");
            if (r == ~0ull) break;
            i = j == std::string::npos ? path.size() : j;
        }
        if (trace) *trace = t;
        return r;
    }
    Bytes read(uint64_t off, size_t len) {
        Bytes b(len);
        f_.clear();
        f_.seekg(std::streamoff(off));
        f_.read(reinterpret_cast<char*>(b.data()), std::streamsize(len));
        if (size_t(f_.gcount()) != len) throw std::runtime_error("read past the end of the image");
        return b;
    }

private:
    std::ifstream f_;    // input only
    struct Ctx { Attr root, alloc; bool hasAlloc = false; };

    // Sizes in the boot sector: a positive value counts clusters, a negative one is -log2(bytes).
    unsigned sizeField(int8_t v) const { return v > 0 ? unsigned(v) * cluster : 1u << -v; }
    Bytes readRuns(const std::vector<Run>& runs, uint64_t from, uint64_t len) {
        Bytes out(len);
        uint64_t vcnStart = 0;
        for (const Run& r : runs) {
            const uint64_t lo = vcnStart * cluster, hi = (vcnStart + r.len) * cluster;
            vcnStart += r.len;
            const uint64_t a = std::max(lo, from), b = std::min(hi, from + len);
            if (a >= b || r.lcn < 0) continue;   // sparse runs read as zeros
            Bytes part = read(uint64_t(r.lcn) * cluster + (a - lo), b - a);
            std::memcpy(&out[a - from], part.data(), part.size());
        }
        return out;
    }
    Ctx ctx(uint64_t dirRec) {
        Bytes rec = record(dirRec);
        if (!(le16(&rec[0x16]) & 2)) throw std::runtime_error("record " + std::to_string(dirRec) + " is not a directory");
        Ctx c;
        bool hasRoot = false;
        for (Attr& a : attrs(rec)) {
            if (a.name != u"$I30") continue;
            if (a.type == kIndexRoot) { c.root = a; hasRoot = true; }
            if (a.type == kIndexAlloc) { c.alloc = a; c.hasAlloc = true; }
        }
        if (!hasRoot || c.root.value.size() < 32) throw std::runtime_error("directory without $INDEX_ROOT");
        return c;
    }
    Bytes indexBlock(const Ctx& c, uint64_t vcn) {
        if (!c.hasAlloc) throw std::runtime_error("subnode pointer but no $INDEX_ALLOCATION");
        const uint64_t unit = idxSize >= cluster ? cluster : 512;
        Bytes b = readRuns(c.alloc.runs, vcn * unit, idxSize);
        if (std::memcmp(b.data(), "INDX", 4)) throw std::runtime_error("index block VCN " + std::to_string(vcn) + ": no INDX signature");
        if (unsigned s = unprotect(b.data(), idxSize))
            throw std::runtime_error("index block VCN " + std::to_string(vcn) + ": update sequence mismatch in stride " + std::to_string(s));
        if (le64(&b[0x10]) != vcn) throw std::runtime_error("index block VCN " + std::to_string(vcn) + " says it is " + std::to_string(le64(&b[0x10])));
        return b;
    }
    static std::u16string keyName(const uint8_t* key) {
        std::u16string n;
        for (unsigned i = 0; i < key[0x40]; i++) n += char16_t(le16(key + 0x42 + 2 * i));
        return n;
    }
    // In-order walk; lo and hi ("" = unbounded) are the keys that must bracket this node.
    void walk(const Ctx& c, const uint8_t* hdr, size_t avail, std::vector<Entry>& out, std::u16string lo, const std::u16string& hi) {
        const uint8_t* end = hdr + std::min<size_t>(le32(hdr + 4), avail);
        const uint8_t* p = hdr + le32(hdr);
        for (int guard = 0; p + 16 <= end && guard < 100000; guard++) {
            const uint16_t len = le16(p + 8), fl = le16(p + 12);
            if (len < 16 || p + len > end) throw std::runtime_error("index entry runs past its node");
            const std::u16string here = fl & 2 ? std::u16string() : keyName(p + 16);
            if (fl & 1) {
                Bytes blk = indexBlock(c, le64(p + len - 8));
                walk(c, &blk[0x18], idxSize - 0x18, out, lo, fl & 2 ? hi : here);
            }
            if (fl & 2) return;
            if ((!lo.empty() && collate(lo, here) >= 0) || (!hi.empty() && collate(here, hi) >= 0))
                std::cerr << "warning: index out of order at \"" << toUtf8(here) << "\"\n";
            out.push_back({here, le64(p) & kRefMask, p[16 + 0x41]});
            lo = here;
            p += len;
        }
        throw std::runtime_error("index node without an end entry");
    }
};

static const char* typeName(uint32_t t) {
    switch (t) {
    case kStdInfo: return "$STANDARD_INFORMATION";
    case kFileName: return "$FILE_NAME";
    case kVolName: return "$VOLUME_NAME";
    case kVolInfo: return "$VOLUME_INFORMATION";
    case kData: return "$DATA";
    case kIndexRoot: return "$INDEX_ROOT";
    case kIndexAlloc: return "$INDEX_ALLOCATION";
    case kBitmap: return "$BITMAP";
    default: return "(other)";
    }
}

static bool dirtyWarning(Ntfs& n) {
    for (const Attr& a : Ntfs::attrs(n.record(3)))
        if (a.type == kVolInfo && a.value.size() >= 12 && (le16(&a.value[10]) & 1)) {
            std::cout << "WARNING: the volume is marked dirty: Windows did not finish unmounting it (crash,\n"
                         "         power cut, or a hibernated Fast Startup shutdown). The metadata may be\n"
                         "         mid-change and $LogFile may hold work that only Windows can replay.\n"
                         "         Reading anyway; nothing is written, by design.\n";
            return true;
        }
    return false;
}

static void info(Ntfs& n) {
    std::printf("NTFS: %u-byte sectors, %u-byte clusters, %llu sectors (%llu clusters)\n", n.sector, n.cluster,
                (unsigned long long)n.sectors, (unsigned long long)(n.sectors * n.sector / n.cluster));
    std::printf("MFT records of %u bytes, index blocks of %u bytes, serial %016llX\n", n.recSize, n.idxSize, (unsigned long long)n.serial);
    std::printf("$MFT at cluster %llu, $MFTMirr at cluster %llu%s; the MFT holds %llu records in",
                (unsigned long long)n.mftLcn, (unsigned long long)n.mirrLcn, n.usedMirror ? " (record 0 taken from the mirror)" : "",
                (unsigned long long)n.records());
    for (const Run& r : n.mftRuns) std::printf(" [%llu clusters at %lld]", (unsigned long long)r.len, (long long)r.lcn);
    std::printf("\n");
    for (const Attr& a : Ntfs::attrs(n.record(3))) {
        if (a.type == kVolName) {
            std::u16string s;
            for (size_t i = 0; i + 1 < a.value.size(); i += 2) s += char16_t(le16(&a.value[i]));
            std::printf("volume name \"%s\"\n", toUtf8(s).c_str());
        }
        if (a.type == kVolInfo && a.value.size() >= 12)
            std::printf("NTFS version %u.%u, volume flags 0x%04x\n", a.value[8], a.value[9], le16(&a.value[10]));
    }
    dirtyWarning(n);
}

static void record(Ntfs& n, uint64_t r) {
    std::string err;
    Bytes b = n.record(r, &err);
    if (b.empty()) { std::printf("record %llu: REFUSED: %s\n", (unsigned long long)r, err.c_str()); return; }
    std::printf("record %llu: flags 0x%04x (%s%s), sequence %u, links %u, %u of %u bytes used, update sequence %u x %u at 0x%x\n",
                (unsigned long long)r, le16(&b[0x16]), le16(&b[0x16]) & 1 ? "in use" : "free",
                le16(&b[0x16]) & 2 ? ", directory" : "", le16(&b[0x10]), le16(&b[0x12]), le32(&b[0x18]), le32(&b[0x1C]),
                le16(&b[b[4]]), le16(&b[6]), le16(&b[4]));
    for (const Attr& a : Ntfs::attrs(b)) {
        std::printf("  0x%02x %-22s %-6s %s", a.type, typeName(a.type), toUtf8(a.name).c_str(), a.nonres ? "non-resident" : "resident");
        if (!a.nonres) std::printf(", %zu bytes\n", a.value.size());
        else {
            std::printf(", size %llu, flags 0x%04x\n", (unsigned long long)a.size, a.flags);
            int64_t prev = 0;
            for (const Run& run : a.runs) {
                if (run.lcn < 0) std::printf("      run: %llu clusters, sparse (no offset field)\n", (unsigned long long)run.len);
                else std::printf("      run: %llu clusters at LCN %lld (offset %+lld)\n", (unsigned long long)run.len, (long long)run.lcn, (long long)(run.lcn - prev));
                if (run.lcn >= 0) prev = run.lcn;
            }
        }
        if (a.type == kFileName && a.value.size() >= 0x42) {
            std::u16string s;
            for (unsigned i = 0; i < a.value[0x40]; i++) s += char16_t(le16(&a.value[0x42 + 2 * i]));
            std::printf("      name \"%s\", namespace %u, parent record %llu\n", toUtf8(s).c_str(), a.value[0x41],
                        (unsigned long long)(le64(&a.value[0]) & kRefMask));
        }
    }
}

static Attr unnamedData(Ntfs& n, uint64_t r) {
    for (const Attr& a : Ntfs::attrs(n.record(r))) {
        if (a.type == 0x20) throw std::runtime_error("record " + std::to_string(r) + " has an $ATTRIBUTE_LIST: not supported, refused");
        if (a.type == kData && a.name.empty()) return a;
    }
    throw std::runtime_error("record " + std::to_string(r) + " has no unnamed $DATA");
}

static int refused = 0;
static void ls(Ntfs& n, uint64_t dir, const std::string& path, bool all) {
    for (const Entry& e : n.list(dir)) {
        if (e.nameSpace == 2) continue;                       // DOS 8.3 alias of a long name
        if (e.name == u"." || (!all && e.ref < 24)) continue;  // records below 24: system files
        const std::string p = path + "/" + toUtf8(e.name);
        std::string err;
        Bytes rec = n.record(e.ref, &err);
        if (rec.empty()) {                                     // say so, skip it, keep going
            std::printf("? %s (record %llu refused: %s)\n", p.c_str(), (unsigned long long)e.ref, err.c_str());
            refused++;
            continue;
        }
        if (le16(&rec[0x16]) & 2) {
            std::printf("d %s\n", p.c_str());
            if (e.ref != 5 && e.ref >= 24) ls(n, e.ref, p, all);
        } else {
            Bytes d = n.data(unnamedData(n, e.ref));
            std::printf("f %s %zu %08x\n", p.c_str(), d.size(), crc32(d));
        }
    }
}

static int check(Ntfs& n) {
    int problems = 0;
    uint64_t inUse = 0;
    Bytes mftBitmap, clusterBitmap = n.data(unnamedData(n, 6));
    for (const Attr& a : Ntfs::attrs(n.rec0))
        if (a.type == kBitmap) mftBitmap = n.data(a);
    std::map<int64_t, uint64_t> owner;
    for (uint64_t c = 0; c < 2; c++) owner[int64_t(c)] = 7;
    for (uint64_t r = 0; r < n.records(); r++) {
        std::string err;
        Bytes b = n.record(r, &err);
        if (b.empty()) {
            std::printf("record %llu: %s\n", (unsigned long long)r, err.c_str());
            problems++;
            if (r != 0) continue;
            b = n.rec0;                                   // carry on with the mirror's copy
            std::printf("record 0: using the $MFTMirr copy for the rest of the check\n");
        }
        const bool used = le16(&b[0x16]) & 1, marked = r / 8 < mftBitmap.size() && (mftBitmap[r / 8] >> r % 8 & 1);
        if (used != marked) { std::printf("record %llu: in-use flag %d but MFT bitmap bit %d\n", (unsigned long long)r, used, marked); problems++; }
        if (!used) continue;
        inUse++;
        for (const Attr& a : Ntfs::attrs(b)) {
            if (r == 7 || !a.nonres) continue;
            for (const Run& run : a.runs)
                for (uint64_t i = 0; run.lcn >= 0 && i < run.len; i++) {
                    auto [it, fresh] = owner.emplace(run.lcn + int64_t(i), r);
                    if (!fresh) { std::printf("cluster %lld claimed by records %llu and %llu\n", (long long)(run.lcn + int64_t(i)), (unsigned long long)it->second, (unsigned long long)r); problems++; }
                }
        }
    }
    const uint64_t clusters = n.sectors * n.sector / n.cluster;
    uint64_t marked = 0;
    for (uint64_t c = 0; c < clusters; c++) {
        const bool bit = clusterBitmap[c / 8] >> c % 8 & 1;
        marked += bit;
        if (bit != owner.count(int64_t(c))) { std::printf("cluster %llu: bitmap %d, owners %zu\n", (unsigned long long)c, bit, owner.count(int64_t(c))); problems++; }
    }
    Bytes mirr = n.read(n.mirrLcn * n.cluster, 4 * n.recSize);
    for (uint64_t r = 0; r < 4; r++) {
        Bytes m(mirr.begin() + long(r * n.recSize), mirr.begin() + long((r + 1) * n.recSize));
        unprotect(m.data(), n.recSize);
        std::string err;
        Bytes live = n.record(r, &err);
        if (!live.empty() && m != live) { std::printf("$MFTMirr record %llu differs from $MFT\n", (unsigned long long)r); problems++; }
    }
    std::printf("%llu records (%llu in use), %llu clusters marked in use and owned, $MFTMirr checked: %d problems\n",
                (unsigned long long)n.records(), (unsigned long long)inUse, (unsigned long long)marked, problems);
    return problems ? 4 : 0;
}

int main(int argc, char** argv) {
    try {
        if (argc < 3) throw std::runtime_error("usage: ntfsread info|record|ls|find|cat|check <img> ...");
        const std::string cmd = argv[1];
        Ntfs n(argv[2]);
        if (cmd == "info") info(n);
        else if (cmd == "record" && argc > 3) record(n, std::stoull(argv[3]));
        else if (cmd == "ls") {
            dirtyWarning(n);
            ls(n, 5, "", argc > 3 && std::string(argv[3]) == "-a");
            return refused ? 3 : 0;
        }
        else if (cmd == "find" && argc > 3) {
            std::string trace;
            const uint64_t r = n.resolve(argv[3], &trace);
            std::cout << trace;
            return r == ~0ull ? 1 : 0;
        } else if (cmd == "cat" && argc > 3) {
            const uint64_t r = n.resolve(argv[3]);
            if (r == ~0ull) throw std::runtime_error(std::string(argv[3]) + ": not found");
            Bytes d = n.data(unnamedData(n, r));
            std::fwrite(d.data(), 1, d.size(), stdout);
        } else if (cmd == "check") return check(n);
        else throw std::runtime_error("unknown command " + cmd);
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "ntfsread: %s\n", e.what());
        return 1;
    }
}
