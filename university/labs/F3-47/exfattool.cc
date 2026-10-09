// exfattool.cc - F3-47: format, write, read and check exFAT volumes (milestone FS4).
// Structures follow the Microsoft "exFAT file system specification" (chapter D1). In this
// build, every field offset below was written from the author's reading notes and checked
// only against this program itself and against libblkid's exFAT probe (see the chapter's
// unverified box): no fsck.exfat, no Windows.
//   exfattool mkfs <img> <MiB> <label>        exfattool ls <img> <dir>
//   exfattool put <img> <path> <hostfile>     exfattool cat <img> <path>
//   exfattool mkdir <img> <path>              exfattool rm <img> <path>
//   exfattool check <img>                     exfattool stress <img> <ops> <seed>
//   exfattool entries <img> <dir>             (raw 32-byte entries of a directory)
//   exfattool fat <img> <cluster> <count>     (FAT entries from <cluster> on)
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Bytes = std::vector<std::uint8_t>;
std::uint32_t le16(const std::uint8_t* p) { return static_cast<std::uint32_t>(p[0] | p[1] << 8); }
std::uint32_t le32(const std::uint8_t* p) { return le16(p) | le16(p + 2) << 16; }
std::uint64_t le64(const std::uint8_t* p) { return le32(p) | static_cast<std::uint64_t>(le32(p + 4)) << 32; }
void put16(std::uint8_t* p, std::uint32_t v) { p[0] = static_cast<std::uint8_t>(v); p[1] = static_cast<std::uint8_t>(v >> 8); }
void put32(std::uint8_t* p, std::uint32_t v) { put16(p, v); put16(p + 2, v >> 16); }
void put64(std::uint8_t* p, std::uint64_t v) { put32(p, static_cast<std::uint32_t>(v)); put32(p + 4, static_cast<std::uint32_t>(v >> 32)); }

constexpr std::uint32_t kSector = 512, kSpcShift = 3, kCluster = kSector << kSpcShift;   // 4 KiB clusters
constexpr std::uint32_t kEoc = 0xFFFFFFFF;
constexpr std::uint8_t kBitmap = 0x81, kUpcase = 0x82, kLabel = 0x83, kFile = 0x85, kStream = 0xC0, kName = 0xC1;
constexpr std::uint8_t kAllocPossible = 0x1, kNoFatChain = 0x2;
constexpr std::uint32_t kAttrDir = 0x10, kAttrArchive = 0x20;
constexpr std::uint32_t kStamp = (44u << 25) | (11u << 21) | (14u << 16);   // 2024-11-14 00:00:00

// The three rotate-and-add checksums of the specification.
std::uint32_t bootChecksum(const Bytes& img)            // sectors 0-10, skipping VolumeFlags and PercentInUse
{
    std::uint32_t c = 0;
    for (std::uint32_t i = 0; i < 11 * kSector; ++i) {
        if (i == 106 || i == 107 || i == 112) continue;
        c = ((c & 1) ? 0x80000000u : 0) + (c >> 1) + img[i];
    }
    return c;
}
std::uint32_t tableChecksum(const Bytes& t)
{
    std::uint32_t c = 0;
    for (std::uint8_t b : t) c = ((c & 1) ? 0x80000000u : 0) + (c >> 1) + b;
    return c;
}
std::uint16_t setChecksum(const std::uint8_t* set, std::size_t entries)   // skips the field itself
{
    std::uint32_t c = 0;
    for (std::size_t i = 0; i < entries * 32; ++i) {
        if (i == 2 || i == 3) continue;
        c = (((c & 1) ? 0x8000u : 0) + (c >> 1) + set[i]) & 0xFFFF;
    }
    return static_cast<std::uint16_t>(c);
}

std::u16string utf16(const std::string& s)               // UTF-8 to UTF-16 (BMP only)
{
    std::u16string out;
    for (std::size_t i = 0; i < s.size();) {
        const std::uint8_t c = static_cast<std::uint8_t>(s[i]);
        if (c < 0x80) { out += c; i += 1; }
        else if ((c & 0xE0) == 0xC0) { out += static_cast<char16_t>((c & 0x1F) << 6 | (s[i + 1] & 0x3F)); i += 2; }
        else if ((c & 0xF0) == 0xE0) { out += static_cast<char16_t>((c & 0x0F) << 12 | (s[i + 1] & 0x3F) << 6 | (s[i + 2] & 0x3F)); i += 3; }
        else throw std::runtime_error("characters outside the BMP are not supported here");
    }
    return out;
}
std::string utf8(const std::u16string& s)
{
    std::string out;
    for (char16_t ch : s) {
        const std::uint32_t c = ch;
        if (c < 0x80) out += static_cast<char>(c);
        else if (c < 0x800) { out += static_cast<char>(0xC0 | c >> 6); out += static_cast<char>(0x80 | (c & 0x3F)); }
        else { out += static_cast<char>(0xE0 | c >> 12); out += static_cast<char>(0x80 | ((c >> 6) & 0x3F)); out += static_cast<char>(0x80 | (c & 0x3F)); }
    }
    return out;
}

// A deliberately small up-case table: 128 entries, a-z mapped to A-Z, everything else to
// itself. Characters beyond the table are treated as their own upper case.
Bytes makeUpcase()
{
    Bytes t(256);
    for (std::uint32_t i = 0; i < 128; ++i) put16(&t[2 * i], (i >= 'a' && i <= 'z') ? i - 32 : i);
    return t;
}

struct Node                                              // a file or directory found in a directory
{
    std::u16string name;
    std::uint32_t attrs = 0, first = 0, firstSlot = 0, entries = 0;
    std::uint32_t tableSum = 0;                          // (up-case table entry only)
    std::uint64_t length = 0;
    bool noFatChain = false;
};

class Volume
{
public:
    explicit Volume(const std::string& path) : path_(path)
    {
        std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(path.c_str(), "rb"), &std::fclose);
        if (!f) throw std::runtime_error("cannot open " + path);
        Bytes buf(1 << 16);
        std::size_t n;
        while ((n = std::fread(buf.data(), 1, buf.size(), f.get())) > 0) img_.insert(img_.end(), buf.begin(), buf.begin() + static_cast<long>(n));
        if (img_.size() < 24 * kSector || std::string(&img_[3], &img_[11]) != "EXFAT   ") throw std::runtime_error("not exFAT");
        if (img_[108] != 9 || img_[109] != kSpcShift) throw std::runtime_error("only 512-byte sectors and 4 KiB clusters here");
        if (bootChecksum(img_) != le32(&img_[11 * kSector])) throw std::runtime_error("boot region checksum mismatch");
        fatOff_ = le32(&img_[80]) * kSector;
        heapOff_ = le32(&img_[88]) * kSector;
        clusters_ = le32(&img_[92]);
        root_ = le32(&img_[96]);
        for (const Node& e : rawEntries(chain(root_), kBitmap)) bitmap_ = e;
        for (const Node& e : rawEntries(chain(root_), kUpcase)) upcase_ = e;
        Bytes t = readStream(upcase_);
        for (std::size_t i = 0; i + 1 < t.size(); i += 2) up_.push_back(static_cast<char16_t>(le16(&t[i])));
    }
    void save() const
    {
        std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(path_.c_str(), "wb"), &std::fclose);
        if (!f || std::fwrite(img_.data(), 1, img_.size(), f.get()) != img_.size()) throw std::runtime_error("cannot write image");
    }

    static void mkfs(const std::string& path, std::uint32_t mib, const std::string& label)
    {
        Bytes img(static_cast<std::size_t>(mib) << 20, 0);
        const std::uint32_t sectors = static_cast<std::uint32_t>(img.size() / kSector);
        const std::uint32_t fatOff = 128, spc = 1u << kSpcShift;
        const std::uint32_t guess = (sectors - fatOff) / spc;
        const std::uint32_t fatLen = ((guess + 2) * 4 + kSector - 1) / kSector;
        const std::uint32_t heap = (fatOff + fatLen + spc - 1) / spc * spc;
        const std::uint32_t count = (sectors - heap) / spc;
        std::uint8_t* b = img.data();
        b[0] = 0xEB; b[1] = 0x76; b[2] = 0x90;
        std::copy_n("EXFAT   ", 8, b + 3);
        put64(b + 72, sectors);                           // VolumeLength
        put32(b + 80, fatOff);
        put32(b + 84, fatLen);
        put32(b + 88, heap);
        put32(b + 92, count);
        put32(b + 96, 4);                                 // root directory: cluster 4
        put32(b + 100, 0x0401F347);                       // VolumeSerialNumber
        put16(b + 104, 0x0100);                           // FileSystemRevision 1.00
        b[108] = 9; b[109] = kSpcShift; b[110] = 1; b[111] = 0x80;
        b[112] = 0xFF;                                    // PercentInUse: not available
        put16(b + 510, 0xAA55);
        for (std::uint32_t s = 1; s <= 8; ++s) put32(b + s * kSector + 508, 0xAA550000);   // extended boot sectors
        const std::uint32_t sum = bootChecksum(img);
        for (std::uint32_t i = 0; i < kSector / 4; ++i) put32(b + 11 * kSector + 4 * i, sum);
        std::copy(img.begin(), img.begin() + 12 * kSector, img.begin() + 12 * kSector);   // backup boot region
        // FAT: media entry, cluster 1, then bitmap (2), up-case (3), root (4), each one cluster
        put32(b + fatOff * kSector, 0xFFFFFFF8);
        for (std::uint32_t c = 1; c <= 4; ++c) put32(b + fatOff * kSector + 4 * c, kEoc);
        auto clusterAt = [&](std::uint32_t c) { return b + (heap + (c - 2) * spc) * kSector; };
        if ((count + 7) / 8 > kCluster) throw std::runtime_error("volume too large for a one-cluster bitmap");
        clusterAt(2)[0] = 0x07;                           // clusters 2, 3, 4 in use
        const Bytes up = makeUpcase();
        std::copy(up.begin(), up.end(), clusterAt(3));
        std::uint8_t* r = clusterAt(4);                   // root directory entries
        const std::u16string l = utf16(label);
        r[0] = kLabel; r[1] = static_cast<std::uint8_t>(std::min<std::size_t>(l.size(), 11));
        for (std::size_t i = 0; i < r[1]; ++i) put16(r + 2 + 2 * i, l[i]);
        r[32] = kBitmap; put32(r + 32 + 20, 2); put64(r + 32 + 24, (count + 7) / 8);
        r[64] = kUpcase; put32(r + 64 + 4, tableChecksum(up)); put32(r + 64 + 20, 3); put64(r + 64 + 24, up.size());
        std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(path.c_str(), "wb"), &std::fclose);
        if (!f || std::fwrite(img.data(), 1, img.size(), f.get()) != img.size()) throw std::runtime_error("cannot write " + path);
        std::printf("formatted %s: %u sectors, FAT at sector %u (%u sectors), cluster heap at sector %u, %u clusters of %u bytes\n",
                    path.c_str(), sectors, fatOff, fatLen, heap, count, kCluster);
    }

    // ---- reading
    std::vector<std::uint32_t> chain(std::uint32_t first, bool contiguous = false, std::uint64_t length = 0) const
    {
        std::vector<std::uint32_t> c;
        if (first == 0) return c;
        if (contiguous) {                                 // NoFatChain: the FAT is not consulted
            for (std::uint64_t i = 0; i < (length + kCluster - 1) / kCluster; ++i) c.push_back(first + static_cast<std::uint32_t>(i));
            return c;
        }
        for (std::uint32_t x = first; x != kEoc; x = fat(x)) {
            if (x < 2 || x >= clusters_ + 2 || c.size() > clusters_) throw std::runtime_error("broken FAT chain");
            c.push_back(x);
        }
        return c;
    }
    Bytes readStream(const Node& n) const
    {
        Bytes out;
        for (std::uint32_t c : chain(n.first, n.noFatChain, n.length)) out.insert(out.end(), at(c), at(c) + kCluster);
        out.resize(n.length);
        return out;
    }
    // Directory entry sets (File + Stream + Names) of the directory made of these clusters.
    std::vector<Node> list(const std::vector<std::uint32_t>& dir, std::vector<std::string>* problems = nullptr) const
    {
        std::vector<Node> out;
        const std::uint32_t slots = static_cast<std::uint32_t>(dir.size()) * (kCluster / 32);
        for (std::uint32_t s = 0; s < slots; ++s) {
            const std::uint8_t* e = slot(dir, s);
            if (e[0] == 0x00) break;                      // end of directory
            if (e[0] != kFile) continue;
            Node n;
            n.entries = e[1] + 1u;
            n.firstSlot = s;
            Bytes set;
            for (std::uint32_t k = 0; k < n.entries && s + k < slots; ++k) set.insert(set.end(), slot(dir, s + k), slot(dir, s + k) + 32);
            const std::uint8_t* st = &set[32];
            n.attrs = le16(e + 4);
            n.noFatChain = (st[1] & kNoFatChain) != 0;
            n.first = le32(st + 20);
            n.length = le64(st + 24);
            for (std::uint32_t k = 2; k < n.entries; ++k) {
                for (int i = 0; i < 15 && n.name.size() < st[3]; ++i) n.name += static_cast<char16_t>(le16(&set[32 * k + 2 + 2 * i]));
            }
            if (problems) {
                if (setChecksum(set.data(), n.entries) != le16(e + 2)) problems->push_back("entry set checksum mismatch: " + utf8(n.name));
                if (nameHash(n.name) != le16(st + 4)) problems->push_back("name hash mismatch: " + utf8(n.name));
            }
            out.push_back(n);
            s += n.entries - 1;
        }
        return out;
    }
    std::vector<std::uint32_t> dirClusters(const std::string& path, Node* self = nullptr) const
    {
        std::vector<std::uint32_t> dir = chain(root_);
        std::size_t pos = 1;
        while (pos < path.size()) {
            std::size_t next = path.find('/', pos);
            if (next == std::string::npos) next = path.size();
            const Node n = find(dir, path.substr(pos, next - pos));
            if (!(n.attrs & kAttrDir)) throw std::runtime_error("not a directory: " + path);
            if (self) *self = n;
            dir = chain(n.first, n.noFatChain, n.length);
            pos = next + 1;
        }
        return dir;
    }
    Node find(const std::vector<std::uint32_t>& dir, const std::string& name) const
    {
        const std::u16string want = upper(utf16(name));
        for (const Node& n : list(dir)) {
            if (upper(n.name) == want) return n;          // names compare without case
        }
        throw std::runtime_error("no such file: " + name);
    }
    Node lookup(const std::string& path) const
    {
        const std::size_t cut = path.rfind('/');
        return find(dirClusters(path.substr(0, cut)), path.substr(cut + 1));
    }

    // ---- writing
    void put(const std::string& path, const Bytes& data, bool directory = false)
    {
        const std::size_t cut = path.rfind('/');
        Node parent;
        std::vector<std::uint32_t> dir = dirClusters(path.substr(0, cut), &parent);
        const std::u16string name = utf16(path.substr(cut + 1));
        if (name.empty() || name.size() > 255) throw std::runtime_error("bad name");
        for (const Node& n : list(dir)) {
            if (upper(n.name) == upper(name)) throw std::runtime_error("exists: " + path);
        }
        const std::uint64_t len = directory ? kCluster : data.size();
        const std::uint32_t need = static_cast<std::uint32_t>((len + kCluster - 1) / kCluster);
        bool contiguous = false;
        std::vector<std::uint32_t> cl = allocate(need, contiguous);
        for (std::size_t i = 0; i < cl.size(); ++i) {
            std::uint8_t* p = at(cl[i]);
            std::fill(p, p + kCluster, 0);
            if (!directory) std::copy(data.begin() + static_cast<long>(i * kCluster),
                                      data.begin() + static_cast<long>(std::min<std::size_t>(data.size(), (i + 1) * kCluster)), p);
        }
        const std::uint32_t entries = 2 + static_cast<std::uint32_t>((name.size() + 14) / 15);
        Bytes set(32 * entries, 0);
        set[0] = kFile; set[1] = static_cast<std::uint8_t>(entries - 1);
        put16(&set[4], directory ? kAttrDir : kAttrArchive);
        put32(&set[8], kStamp); put32(&set[12], kStamp); put32(&set[16], kStamp);
        std::uint8_t* st = &set[32];
        st[0] = kStream;
        st[1] = static_cast<std::uint8_t>((cl.empty() ? 0 : kAllocPossible) | (contiguous ? kNoFatChain : 0));
#ifdef FORENSIC_ALWAYS_NOFATCHAIN
        st[1] = static_cast<std::uint8_t>(cl.empty() ? 0 : kAllocPossible | kNoFatChain);   // the forensic lab's build
#endif
        st[3] = static_cast<std::uint8_t>(name.size());
        put16(st + 4, nameHash(name));
        put64(st + 8, len);                               // ValidDataLength
        put32(st + 20, cl.empty() ? 0 : cl[0]);
        put64(st + 24, len);                              // DataLength
        for (std::size_t i = 0; i < name.size(); ++i) {
            std::uint8_t* ne = &set[32 * (2 + i / 15)];
            ne[0] = kName;
            put16(ne + 2 + 2 * (i % 15), name[i]);
        }
        put16(&set[2], setChecksum(set.data(), entries));
        std::uint32_t s = freeSlots(dir, entries, parent, path.substr(0, cut));
        for (std::uint32_t k = 0; k < entries; ++k) std::copy(&set[32 * k], &set[32 * k] + 32, slot(dir, s + k));
    }
    void remove(const std::string& path)
    {
        const std::size_t cut = path.rfind('/');
        const std::vector<std::uint32_t> dir = dirClusters(path.substr(0, cut));
        const Node n = find(dir, path.substr(cut + 1));
        if ((n.attrs & kAttrDir) && !list(chain(n.first, n.noFatChain, n.length)).empty()) throw std::runtime_error("directory not empty");
        for (std::uint32_t k = 0; k < n.entries; ++k) slot(dir, n.firstSlot + k)[0] &= 0x7F;   // InUse = 0
        for (std::uint32_t c : chain(n.first, n.noFatChain, n.length)) {
            setBit(c, false);
            if (!n.noFatChain) setFat(c, 0);
        }
    }

    // ---- checking: boot regions, entry sets, bitmap against what the tree uses
    int check()
    {
        std::vector<std::string> problems;
        if (!std::equal(img_.begin(), img_.begin() + 12 * kSector, img_.begin() + 12 * kSector)) problems.push_back("backup boot region differs");
        Bytes t = readStream(upcase_);
        if (tableChecksum(t) != upcaseSum()) problems.push_back("up-case table checksum mismatch");
        std::vector<int> used(clusters_ + 2, 0);
        auto mark = [&](const std::vector<std::uint32_t>& c, const std::string& who) {
            for (std::uint32_t x : c) {
                if (used[x]++) problems.push_back("cluster " + std::to_string(x) + " used twice (again by " + who + ")");
            }
        };
        mark(chain(bitmap_.first, false, 0), "allocation bitmap");
        mark(chain(upcase_.first, false, 0), "up-case table");
        unsigned long files = 0, dirs = 0;
        walk(chain(root_), "", mark, problems, files, dirs);
        unsigned long mismatched = 0;
        for (std::uint32_t c = 2; c < clusters_ + 2; ++c) {
            if (bit(c) != (used[c] > 0)) ++mismatched;
        }
        if (mismatched) problems.push_back(std::to_string(mismatched) + " clusters where the bitmap disagrees with the tree");
        std::printf("check: %lu files, %lu directories, %lu clusters in use; boot checksum 0x%08x ok\n",
                    files, dirs, static_cast<unsigned long>(std::count_if(used.begin(), used.end(), [](int u) { return u > 0; })),
                    le32(&img_[11 * kSector]));
        for (const std::string& p : problems) std::printf("  PROBLEM: %s\n", p.c_str());
        std::printf("check: %zu problem(s)\n", problems.size());
        return problems.empty() ? 0 : 4;
    }

    void dumpEntries(const std::string& path) const
    {
        const std::vector<std::uint32_t> dir = dirClusters(path);
        for (std::uint32_t s = 0; s < dir.size() * (kCluster / 32); ++s) {
            const std::uint8_t* e = slot(dir, s);
            if (e[0] == 0) break;
            std::printf("slot %3u:", s);
            for (int i = 0; i < 32; ++i) std::printf(" %02x", e[i]);
            std::printf("\n");
        }
    }
    std::uint32_t fat(std::uint32_t c) const { return le32(&img_[fatOff_ + 4 * c]); }
    std::u16string upper(std::u16string s) const
    {
        for (char16_t& c : s) {
            if (c < up_.size()) c = up_[c];
        }
        return s;
    }
    std::uint16_t nameHash(const std::u16string& name) const
    {
        std::uint32_t h = 0;
        for (char16_t ch : upper(name)) {
            for (std::uint32_t b : {static_cast<std::uint32_t>(ch & 0xFF), static_cast<std::uint32_t>(ch >> 8)})
                h = (((h & 1) ? 0x8000u : 0) + (h >> 1) + b) & 0xFFFF;
        }
        return static_cast<std::uint16_t>(h);
    }

private:
    std::uint8_t* at(std::uint32_t c) { return &img_[heapOff_ + static_cast<std::size_t>(c - 2) * kCluster]; }
    const std::uint8_t* at(std::uint32_t c) const { return &img_[heapOff_ + static_cast<std::size_t>(c - 2) * kCluster]; }
    std::uint8_t* slot(const std::vector<std::uint32_t>& dir, std::uint32_t s) { return at(dir[s / (kCluster / 32)]) + 32 * (s % (kCluster / 32)); }
    const std::uint8_t* slot(const std::vector<std::uint32_t>& dir, std::uint32_t s) const { return at(dir[s / (kCluster / 32)]) + 32 * (s % (kCluster / 32)); }
    void setFat(std::uint32_t c, std::uint32_t v) { put32(&img_[fatOff_ + 4 * c], v); }
    bool bit(std::uint32_t c) const { return at(bitmap_.first)[(c - 2) / 8] & (1u << ((c - 2) % 8)); }
    void setBit(std::uint32_t c, bool on)
    {
        std::uint8_t& b = at(bitmap_.first)[(c - 2) / 8];
        b = static_cast<std::uint8_t>(on ? b | (1u << ((c - 2) % 8)) : b & ~(1u << ((c - 2) % 8)));
    }
    std::uint32_t upcaseSum() const
    {
        for (const Node& e : rawEntries(chain(root_), kUpcase)) return e.tableSum;
        return 0;
    }
    // System entries of the root directory (bitmap, up-case): first cluster and length.
    std::vector<Node> rawEntries(const std::vector<std::uint32_t>& dir, std::uint8_t type) const
    {
        std::vector<Node> out;
        for (std::uint32_t s = 0; s < dir.size() * (kCluster / 32); ++s) {
            const std::uint8_t* e = slot(dir, s);
            if (e[0] == 0) break;
            if (e[0] != type) continue;
            Node n;
            n.first = le32(e + 20);
            n.length = le64(e + 24);
            n.tableSum = le32(e + 4);                     // (up-case table: TableChecksum)
            out.push_back(n);
        }
        if (out.empty()) throw std::runtime_error("missing system entry in the root directory");
        return out;
    }
    // Finds `need` free clusters; one contiguous run if possible (then no FAT chain is written).
    std::vector<std::uint32_t> allocate(std::uint32_t need, bool& contiguous)
    {
        std::vector<std::uint32_t> got;
        contiguous = false;
        if (need == 0) return got;
        for (std::uint32_t c = 2, run = 0; c < clusters_ + 2; ++c) {
            run = bit(c) ? 0 : run + 1;
            if (run == need) {
                for (std::uint32_t x = c + 1 - need; x <= c; ++x) got.push_back(x);
                contiguous = true;
                break;
            }
        }
        for (std::uint32_t c = 2; !contiguous && c < clusters_ + 2 && got.size() < need; ++c) {
            if (!bit(c)) got.push_back(c);
        }
        if (got.size() < need) throw std::runtime_error("volume full");
        for (std::size_t i = 0; i < got.size(); ++i) {
            setBit(got[i], true);
            if (!contiguous) setFat(got[i], i + 1 < got.size() ? got[i + 1] : kEoc);   // a FAT chain
        }
        return got;
    }
    std::uint32_t freeSlots(std::vector<std::uint32_t>& dir, std::uint32_t need, Node& parent, const std::string& parentPath)
    {
        for (;;) {
            const std::uint32_t slots = static_cast<std::uint32_t>(dir.size()) * (kCluster / 32);
            for (std::uint32_t s = 0, run = 0; s < slots; ++s) {
                run = (slot(dir, s)[0] & 0x80) ? 0 : run + 1;  // 0x00 (end) or deleted: free
                if (run == need) return s + 1 - need;
            }
            bool contiguous = false;                          // grow the directory by one cluster
            const std::uint32_t c = allocate(1, contiguous)[0];
            std::fill(at(c), at(c) + kCluster, 0);
            if (parentPath.empty()) {
                setFat(dir.back(), c);                        // the root always uses the FAT
                setFat(c, kEoc);
            } else {
                if (parent.noFatChain) {                       // convert the directory to a FAT chain
                    for (std::size_t i = 0; i + 1 < dir.size(); ++i) setFat(dir[i], dir[i + 1]);
                }
                setFat(dir.back(), c);
                setFat(c, kEoc);
                updateLength(parentPath, parent.length + kCluster);
                parent.length += kCluster;
                parent.noFatChain = false;
            }
            dir.push_back(c);
        }
    }
    void updateLength(const std::string& dirPath, std::uint64_t len)
    {
        const std::size_t cut = dirPath.rfind('/');
        const std::vector<std::uint32_t> pdir = dirClusters(dirPath.substr(0, cut));
        const Node n = find(pdir, dirPath.substr(cut + 1));
        std::uint8_t* st = slot(pdir, n.firstSlot + 1);
        st[1] = static_cast<std::uint8_t>(st[1] & ~kNoFatChain);
        put64(st + 8, len);
        put64(st + 24, len);
        Bytes set;
        for (std::uint32_t k = 0; k < n.entries; ++k) set.insert(set.end(), slot(pdir, n.firstSlot + k), slot(pdir, n.firstSlot + k) + 32);
        put16(slot(pdir, n.firstSlot) + 2, setChecksum(set.data(), n.entries));
    }
    template <class Mark>
    void walk(const std::vector<std::uint32_t>& dir, const std::string& path, Mark& mark, std::vector<std::string>& problems,
              unsigned long& files, unsigned long& dirs)
    {
        mark(dir, "directory " + (path.empty() ? std::string("/") : path));
        for (const Node& n : list(dir, &problems)) {
            const std::string p = path + "/" + utf8(n.name);
            const std::vector<std::uint32_t> c = chain(n.first, n.noFatChain, n.length);
            if (n.attrs & kAttrDir) {
                ++dirs;
                walk(c, p, mark, problems, files, dirs);
            } else {
                ++files;
                mark(c, p);
            }
        }
    }

    std::string path_;
    Bytes img_;
    std::uint32_t fatOff_ = 0, heapOff_ = 0, clusters_ = 0, root_ = 0;
    Node bitmap_, upcase_;
    std::u16string up_;
};

Bytes readHost(const std::string& path)
{
    std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(path.c_str(), "rb"), &std::fclose);
    if (!f) throw std::runtime_error("cannot open " + path);
    Bytes b;
    int c;
    while ((c = std::fgetc(f.get())) != EOF) b.push_back(static_cast<std::uint8_t>(c));
    return b;
}

int stress(const std::string& img, int ops, unsigned seed)
{
    Volume v(img);
    std::mt19937 rng(seed);
    std::map<std::string, Bytes> model;
    std::vector<std::string> dirs{""};
    const std::vector<std::string> stems = {"photo", "Grüße", "naïve", "résumé", "Ωmega", "data", "Ñandú"};
    std::map<std::string, int> done;
    for (int i = 0; i < ops; ++i) {
        const unsigned r = rng() % 100;
        if (r < 50 || model.empty()) {
            const std::string p = dirs[rng() % dirs.size()] + "/" + stems[rng() % stems.size()] + std::to_string(i) + ".bin";
            Bytes d(rng() % 40000);
            for (auto& x : d) x = static_cast<std::uint8_t>(rng() >> 24);
            try {
                v.put(p, d);
                model[p] = d;
                ++done["create"];
            } catch (const std::runtime_error&) {
                ++done["create refused: volume full"];
            }
        } else if (r < 90) {
            auto it = model.begin();
            std::advance(it, rng() % model.size());
            v.remove(it->first);
            model.erase(it);
            ++done["delete"];
        } else if (dirs.size() < 20) {
            const std::string p = dirs[rng() % dirs.size()] + "/dir" + std::to_string(i);
            v.put(p, {}, true);
            dirs.push_back(p);
            ++done["mkdir"];
        }
    }
    unsigned long bad = 0;
    for (const auto& [p, d] : model) {
        if (v.readStream(v.lookup(p)) != d) ++bad;
    }
    std::printf("stress seed %u, %d operations:", seed, ops);
    for (const auto& [what, n] : done) std::printf(" %s %d;", what.c_str(), n);
    std::printf("\nfiles read back and compared: %zu, different: %lu\n", model.size(), bad);
    v.save();
    return bad ? 1 : v.check();
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const std::vector<std::string> a(argv + 1, argv + argc);
        if (a.size() == 4 && a[0] == "mkfs") {
            Volume::mkfs(a[1], static_cast<std::uint32_t>(std::stoul(a[2])), a[3]);
        } else if (a.size() == 4 && a[0] == "put") {
            Volume v(a[1]);
            v.put(a[2], readHost(a[3]));
            v.save();
        } else if (a.size() == 3 && a[0] == "mkdir") {
            Volume v(a[1]);
            v.put(a[2], {}, true);
            v.save();
        } else if (a.size() == 3 && a[0] == "rm") {
            Volume v(a[1]);
            v.remove(a[2]);
            v.save();
        } else if (a.size() == 3 && a[0] == "ls") {
            Volume v(a[1]);
            for (const Node& n : v.list(v.dirClusters(a[2]))) {
                std::printf("%s %10llu  cluster %-5u %-11s %s\n", (n.attrs & kAttrDir) ? "d" : "-",
                            static_cast<unsigned long long>(n.length), n.first, n.noFatChain ? "contiguous" : "FAT chain",
                            utf8(n.name).c_str());
            }
        } else if (a.size() == 3 && a[0] == "cat") {
            Volume v(a[1]);
            const Bytes d = v.readStream(v.lookup(a[2]));
            std::fwrite(d.data(), 1, d.size(), stdout);
        } else if (a.size() == 3 && a[0] == "entries") {
            Volume(a[1]).dumpEntries(a[2]);
        } else if (a.size() == 4 && a[0] == "fat") {
            Volume v(a[1]);
            const std::uint32_t c0 = static_cast<std::uint32_t>(std::stoul(a[2]));
            for (std::uint32_t c = c0; c < c0 + std::stoul(a[3]); ++c) {
                const std::uint32_t x = v.fat(c);
                if (x == kEoc) std::printf("FAT[%u] = end of chain\n", c);
                else if (x == 0) std::printf("FAT[%u] = 0 (no chain entry)\n", c);
                else std::printf("FAT[%u] = %u\n", c, x);
            }
        } else if (a.size() == 2 && a[0] == "check") {
            return Volume(a[1]).check();
        } else if (a.size() == 4 && a[0] == "stress") {
            return stress(a[1], std::stoi(a[2]), static_cast<unsigned>(std::stoul(a[3])));
        } else {
            std::fprintf(stderr, "usage: see the comment at the top of exfattool.cc\n");
            return 2;
        }
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "exfattool: %s\n", e.what());
        return 1;
    }
}
