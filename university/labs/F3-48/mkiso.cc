// mkiso.cc - F3-48: build an ISO 9660 image from a host directory, with Rock Ridge (POSIX
// names, modes, symbolic links), a Joliet tree (Unicode names) and an El Torito boot catalog
// (a BIOS no-emulation entry and, optionally, a UEFI entry pointing at a FAT image).
// Structures: ECMA-119; the Joliet, SUSP/RRIP and El Torito documents (chapter D1-D4).
//   mkiso <out.iso> <dir> <volume id> [<bios boot image> [<efi fat image>]]
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <tuple>
#include <vector>

namespace {

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;
constexpr std::uint32_t kSector = 2048;

void le16(std::uint8_t* p, std::uint32_t v) { p[0] = static_cast<std::uint8_t>(v); p[1] = static_cast<std::uint8_t>(v >> 8); }
void le32(std::uint8_t* p, std::uint32_t v) { le16(p, v); le16(p + 2, v >> 16); }
void be16(std::uint8_t* p, std::uint32_t v) { p[0] = static_cast<std::uint8_t>(v >> 8); p[1] = static_cast<std::uint8_t>(v); }
void be32(std::uint8_t* p, std::uint32_t v) { be16(p, v >> 16); be16(p + 2, v); }
void both16(std::uint8_t* p, std::uint32_t v) { le16(p, v); be16(p + 2, v); }   // "both-byte orders"
void both32(std::uint8_t* p, std::uint32_t v) { le32(p, v); be32(p + 4, v); }
std::uint32_t sectors(std::uint64_t bytes) { return static_cast<std::uint32_t>((bytes + kSector - 1) / kSector); }

Bytes readHost(const fs::path& p)
{
    std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(p.c_str(), "rb"), &std::fclose);
    if (!f) throw std::runtime_error("cannot open " + p.string());
    Bytes b;
    for (int c; (c = std::fgetc(f.get())) != EOF;) b.push_back(static_cast<std::uint8_t>(c));
    return b;
}

std::u16string ucs2(const std::string& s)
{
    std::u16string out;
    for (std::size_t i = 0; i < s.size();) {
        const std::uint8_t c = static_cast<std::uint8_t>(s[i]);
        if (c < 0x80) { out += c; i += 1; }
        else if ((c & 0xE0) == 0xC0) { out += static_cast<char16_t>((c & 0x1F) << 6 | (s[i + 1] & 0x3F)); i += 2; }
        else { out += static_cast<char16_t>((c & 0x0F) << 12 | (s[i + 1] & 0x3F) << 6 | (s[i + 2] & 0x3F)); i += 3; }
    }
    return out;
}
Bytes ucs2be(const std::u16string& s)
{
    Bytes b;
    for (char16_t c : s) { b.push_back(static_cast<std::uint8_t>(c >> 8)); b.push_back(static_cast<std::uint8_t>(c)); }
    return b;
}

struct Node
{
    std::string name;                 // host name (Rock Ridge NM)
    Bytes isoId, jolietId;            // identifiers in the primary and Joliet trees
    fs::path host;
    bool dir = false, link = false;
    std::string target;
    std::uint32_t mode = 0;
    Bytes data;
    std::vector<Node> kids;
    std::uint32_t extent = 0, size = 0, jExtent = 0, jSize = 0, num = 0, parentNum = 0;
};

// ISO 9660 level 1 identifier: upper-case d-characters, 8.3, ";1" on files, unique per directory.
Bytes levelOneId(const std::string& name, bool dir, std::set<std::string>& used)
{
    std::string base = name, ext;
    const std::size_t dot = name.rfind('.');
    if (!dir && dot != std::string::npos && dot > 0) { base = name.substr(0, dot); ext = name.substr(dot + 1); }
    auto clean = [](std::string s, std::size_t n) {
        std::string o;
        for (char c : s) {
            const char u = (c >= 'a' && c <= 'z') ? static_cast<char>(c - 32) : c;
            o += ((u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '_') ? u : '_';
        }
        return o.substr(0, n);
    };
    base = clean(base, 8);
    ext = clean(ext, 3);
    std::string id = base + (dir ? "" : "." + ext);
    for (int k = 1; used.count(id); ++k) {                     // make it unique
        const std::string suffix = std::to_string(k);
        id = base.substr(0, 8 - suffix.size()) + suffix + (dir ? "" : "." + ext);
    }
    used.insert(id);
    if (!dir) id += ";1";
    return Bytes(id.begin(), id.end());
}

Node scan(const fs::path& p, const std::string& name)
{
    Node n;
    n.name = name;
    n.host = p;
    struct stat st{};
    if (::lstat(p.c_str(), &st) != 0) throw std::runtime_error("cannot stat " + p.string());
    n.mode = st.st_mode;
    n.link = S_ISLNK(st.st_mode);
    n.dir = S_ISDIR(st.st_mode);
    if (n.link) n.target = fs::read_symlink(p).string();
    else if (!n.dir) n.data = readHost(p);
    if (n.dir) {
        std::vector<fs::path> names;
        for (const auto& e : fs::directory_iterator(p)) names.push_back(e.path());
        std::sort(names.begin(), names.end());
        std::set<std::string> used;
        for (const auto& c : names) {
            Node k = scan(c, c.filename().string());
            k.isoId = levelOneId(k.name, k.dir, used);
            Bytes j = ucs2be(ucs2(k.name.substr(0, 64)) + (k.dir ? u"" : u";1"));
            k.jolietId = j;
            n.kids.push_back(std::move(k));
        }
        std::sort(n.kids.begin(), n.kids.end(), [](const Node& a, const Node& b) { return a.isoId < b.isoId; });
    }
    return n;
}

// System Use entries (SUSP) carrying Rock Ridge: PX (mode, links, uid, gid), NM, SL.
Bytes px(std::uint32_t mode, std::uint32_t links)
{
    Bytes e(36, 0);
    e[0] = 'P'; e[1] = 'X'; e[2] = 36; e[3] = 1;
    both32(&e[4], mode);
    both32(&e[12], links);
    return e;                                                  // uid and gid stay 0
}
Bytes nm(const std::string& name)
{
    Bytes e = {'N', 'M', static_cast<std::uint8_t>(5 + name.size()), 1, 0};
    e.insert(e.end(), name.begin(), name.end());
    return e;
}
Bytes sl(const std::string& target)
{
    Bytes comps;
    std::size_t pos = 0;
    if (!target.empty() && target[0] == '/') { comps.insert(comps.end(), {0x08, 0}); pos = 1; }   // ROOT
    while (pos <= target.size()) {
        std::size_t next = target.find('/', pos);
        if (next == std::string::npos) next = target.size();
        const std::string c = target.substr(pos, next - pos);
        if (c == ".") comps.insert(comps.end(), {0x02, 0});
        else if (c == "..") comps.insert(comps.end(), {0x04, 0});
        else if (!c.empty()) { comps.push_back(0); comps.push_back(static_cast<std::uint8_t>(c.size())); comps.insert(comps.end(), c.begin(), c.end()); }
        pos = next + 1;
    }
    Bytes e = {'S', 'L', static_cast<std::uint8_t>(5 + comps.size()), 1, 0};
    e.insert(e.end(), comps.begin(), comps.end());
    return e;
}
Bytes rootSu()                                                 // SP and ER, then PX
{
    Bytes e = {'S', 'P', 7, 1, 0xBE, 0xEF, 0};
    const std::string id = "RRIP_1991A", des = "ROCK RIDGE (OS401 LAB IMAGE)", src = "SEE CHAPTER F3-48 SOURCES";
    Bytes er = {'E', 'R', static_cast<std::uint8_t>(8 + id.size() + des.size() + src.size()), 1,
                static_cast<std::uint8_t>(id.size()), static_cast<std::uint8_t>(des.size()), static_cast<std::uint8_t>(src.size()), 1};
    er.insert(er.end(), id.begin(), id.end());
    er.insert(er.end(), des.begin(), des.end());
    er.insert(er.end(), src.begin(), src.end());
    e.insert(e.end(), er.begin(), er.end());
    return e;
}

Bytes record(const Bytes& id, std::uint32_t extent, std::uint32_t size, bool dir, const Bytes& su)
{
    const std::size_t pad = (id.size() % 2 == 0) ? 1 : 0;      // System Use starts at an even offset
    Bytes r(33 + id.size() + pad, 0);
    r[1] = 0;
    both32(&r[2], extent);
    both32(&r[10], size);
    const std::uint8_t date[7] = {124, 11, 14, 0, 0, 0, 0};    // 2024-11-14 00:00:00 UTC
    std::copy(date, date + 7, &r[18]);
    r[25] = dir ? 0x02 : 0x00;                                 // file flags: directory
    both16(&r[28], 1);                                         // volume sequence number
    r[32] = static_cast<std::uint8_t>(id.size());
    std::copy(id.begin(), id.end(), &r[33]);
    r.insert(r.end(), su.begin(), su.end());
    if (r.size() > 255) throw std::runtime_error("directory record too long");
    r[0] = static_cast<std::uint8_t>(r.size());
    return r;
}

struct Iso
{
    Node root;
    std::vector<Node*> dirs;                                   // breadth-first order = path table order
    bool joliet = false;

    std::vector<Bytes> records(const Node& d, const Node& parent, bool jol) const
    {
        std::vector<Bytes> r;
        const bool isRoot = &d == &root;
        const std::uint32_t links = 2 + static_cast<std::uint32_t>(std::count_if(d.kids.begin(), d.kids.end(), [](const Node& k) { return k.dir; }));
        Bytes su0 = jol ? Bytes{} : (isRoot ? rootSu() : Bytes{});
        Bytes p0 = jol ? Bytes{} : px(d.mode, links);
        su0.insert(su0.end(), p0.begin(), p0.end());
        r.push_back(record({0}, jol ? d.jExtent : d.extent, jol ? d.jSize : d.size, true, su0));
        r.push_back(record({1}, jol ? parent.jExtent : parent.extent, jol ? parent.jSize : parent.size, true,
                           jol ? Bytes{} : px(parent.mode, 2)));
        for (const Node& k : d.kids) {
            if (jol && k.link) continue;                       // Joliet cannot express a symbolic link
            Bytes su;
            if (!jol) {
                su = px(k.mode, k.dir ? 2 : 1);
                Bytes n = nm(k.name);
                su.insert(su.end(), n.begin(), n.end());
                if (k.link) { Bytes s = sl(k.target); su.insert(su.end(), s.begin(), s.end()); }
            }
            // a file's data has one extent, shared by both trees; each tree has its own directories
            r.push_back(record(jol ? k.jolietId : k.isoId, k.dir ? (jol ? k.jExtent : k.extent) : k.extent,
                               k.dir ? (jol ? k.jSize : k.size) : static_cast<std::uint32_t>(k.data.size()), k.dir, su));
        }
        return r;
    }
    static std::uint32_t packedSize(const std::vector<Bytes>& recs)
    {
        std::uint32_t off = 0;
        for (const Bytes& b : recs) {
            if (off % kSector + b.size() > kSector) off = (off / kSector + 1) * kSector;   // never cross a sector
            off += static_cast<std::uint32_t>(b.size());
        }
        return sectors(off) * kSector;
    }
    static void pack(std::uint8_t* out, const std::vector<Bytes>& recs)
    {
        std::uint32_t off = 0;
        for (const Bytes& b : recs) {
            if (off % kSector + b.size() > kSector) off = (off / kSector + 1) * kSector;
            std::copy(b.begin(), b.end(), out + off);
            off += static_cast<std::uint32_t>(b.size());
        }
    }
    Bytes pathTable(bool jol, bool bigEndian) const
    {
        Bytes t;
        for (const Node* d : dirs) {
            Bytes id = d == &root ? Bytes{0} : (jol ? d->jolietId : d->isoId);
            Bytes e(8 + id.size() + (id.size() % 2), 0);
            e[0] = static_cast<std::uint8_t>(id.size());
            (bigEndian ? be32 : le32)(&e[2], jol ? d->jExtent : d->extent);
            (bigEndian ? be16 : le16)(&e[6], d->parentNum);
            std::copy(id.begin(), id.end(), &e[8]);
            t.insert(t.end(), e.begin(), e.end());
        }
        return t;
    }
};

void collect(Iso& iso, Node& d)
{
    std::vector<Node*> level = {&d};
    d.num = d.parentNum = 1;
    while (!level.empty()) {
        std::vector<Node*> next;
        for (Node* n : level) {
            iso.dirs.push_back(n);
            n->num = static_cast<std::uint32_t>(iso.dirs.size());
        }
        for (Node* n : level) {
            for (Node& k : n->kids) {
                if (k.dir) { k.parentNum = n->num; next.push_back(&k); }
            }
        }
        level = next;
    }
}

void volumeDescriptor(std::uint8_t* v, std::uint8_t type, const Iso& iso, bool jol, const std::string& volId,
                      std::uint32_t total, std::uint32_t ptSize, std::uint32_t lpt, std::uint32_t mpt)
{
    v[0] = type;
    std::copy_n("CD001", 5, v + 1);
    v[6] = 1;
    auto text = [&](std::uint32_t off, std::uint32_t len, const std::string& s) {
        if (jol) {                                             // UCS-2 big-endian, space padded
            Bytes u = ucs2be(ucs2(s));
            for (std::uint32_t i = 0; i < len; i += 2) { v[off + i] = 0; v[off + i + 1] = ' '; }
            std::copy_n(u.begin(), std::min<std::size_t>(u.size(), len), v + off);
        } else {
            std::fill(v + off, v + off + len, ' ');
            std::copy_n(s.begin(), std::min<std::size_t>(s.size(), len), v + off);
        }
    };
    text(8, 32, "");                                           // system identifier
    text(40, 32, volId);                                       // volume identifier
    both32(v + 80, total);                                     // volume space size
    if (jol) { v[88] = '%'; v[89] = '/'; v[90] = 'E'; }        // escape sequence: UCS-2 level 3
    both16(v + 120, 1);
    both16(v + 124, 1);
    both16(v + 128, kSector);
    both32(v + 132, ptSize);
    le32(v + 140, lpt);
    be32(v + 148, mpt);
    const Node& r = iso.root;
    Bytes rr = record({0}, jol ? r.jExtent : r.extent, jol ? r.jSize : r.size, true, {});
    std::copy(rr.begin(), rr.end(), v + 156);
    for (std::uint32_t off : {190u, 318u, 446u, 574u}) text(off, 128, off == 574 ? "OS401 MKISO" : "");
    for (std::uint32_t off : {702u, 739u, 776u}) text(off, 37, "");
    for (std::uint32_t off : {813u, 830u, 847u, 864u}) {       // dates: "2024111400000000" + zone 0
        std::copy_n(off == 813 || off == 830 ? "2024111400000000" : "0000000000000000", 16, v + off);
        v[off + 16] = 0;
    }
    v[881] = 1;                                                // file structure version
}

} // namespace

int main(int argc, char** argv)
{
    try {
        if (argc < 4 || argc > 6) throw std::runtime_error("usage: mkiso <out.iso> <dir> <volume id> [<bios image> [<efi image>]]");
        Iso iso;
        iso.root = scan(argv[2], "");
        const Bytes bios = argc >= 5 ? readHost(argv[4]) : Bytes{};
        const Bytes efi = argc >= 6 ? readHost(argv[5]) : Bytes{};
        collect(iso, iso.root);
        // Layout: system area 0-15, descriptors from 16, then catalog, path tables, directories, data.
        std::uint32_t next = 16;
        const std::uint32_t pvd = next++, br = bios.empty() ? 0 : next++, svd = next++, term = next++;
        const std::uint32_t catalog = bios.empty() ? 0 : next++;
        const std::uint32_t ptP = static_cast<std::uint32_t>(iso.pathTable(false, false).size()); // sizes do not depend on extents
        const std::uint32_t ptJ = static_cast<std::uint32_t>(iso.pathTable(true, false).size());
        const std::uint32_t lP = next; next += sectors(ptP);
        const std::uint32_t mP = next; next += sectors(ptP);
        const std::uint32_t lJ = next; next += sectors(ptJ);
        const std::uint32_t mJ = next; next += sectors(ptJ);
        for (Node* d : iso.dirs) { d->size = Iso::packedSize(iso.records(*d, *d, false)); d->extent = next; next += d->size / kSector; }
        for (Node* d : iso.dirs) { d->jSize = Iso::packedSize(iso.records(*d, *d, true)); d->jExtent = next; next += d->jSize / kSector; }
        for (Node* d : iso.dirs) {
            for (Node& k : d->kids) {
                if (!k.dir && !k.link && !k.data.empty()) { k.extent = next; next += sectors(k.data.size()); }
            }
        }
        const std::uint32_t biosAt = next; next += sectors(bios.size());
        const std::uint32_t efiAt = next; next += sectors(efi.size());
        const std::uint32_t total = next;
        Bytes img(static_cast<std::size_t>(total) * kSector, 0);
        auto sec = [&](std::uint32_t s) { return &img[static_cast<std::size_t>(s) * kSector]; };

        volumeDescriptor(sec(pvd), 1, iso, false, argv[3], total, ptP, lP, mP);
        volumeDescriptor(sec(svd), 2, iso, true, argv[3], total, ptJ, lJ, mJ);
        sec(term)[0] = 255; std::copy_n("CD001", 5, sec(term) + 1); sec(term)[6] = 1;
        for (auto [jol, l, m] : {std::tuple{false, lP, mP}, std::tuple{true, lJ, mJ}}) {
            Bytes lt = iso.pathTable(jol, false), mt = iso.pathTable(jol, true);
            std::copy(lt.begin(), lt.end(), sec(l));
            std::copy(mt.begin(), mt.end(), sec(m));
        }
        for (Node* d : iso.dirs) {
            const Node* parent = iso.dirs[d->parentNum - 1];
            Iso::pack(sec(d->extent), iso.records(*d, *parent, false));
            Iso::pack(sec(d->jExtent), iso.records(*d, *parent, true));
            for (Node& k : d->kids) {
                if (!k.dir && !k.link) std::copy(k.data.begin(), k.data.end(), sec(k.extent));
            }
        }
        if (!bios.empty()) {                                   // El Torito
            std::uint8_t* b = sec(br);
            b[0] = 0; std::copy_n("CD001", 5, b + 1); b[6] = 1;
            const std::string sys = "EL TORITO SPECIFICATION";
            std::copy(sys.begin(), sys.end(), b + 7);
            le32(b + 0x47, catalog);
            std::uint8_t* c = sec(catalog);
            c[0] = 0x01;                                       // validation entry, platform 0 = 80x86
            std::copy_n("OS401 MKISO", 11, c + 4);
            c[30] = 0x55; c[31] = 0xAA;
            std::uint32_t sum = 0;
            for (int i = 0; i < 32; i += 2) sum += static_cast<std::uint32_t>(c[i] | c[i + 1] << 8);
            le16(c + 28, (0x10000 - (sum & 0xFFFF)) & 0xFFFF);  // all 16 words must add up to 0
            c[32] = 0x88;                                      // default entry: bootable
            c[33] = 0;                                         // no emulation
            le16(c + 34, 0);                                   // load segment 0 = the default 0x7C0
            le16(c + 38, sectors(bios.size()) * 4);            // in 512-byte virtual sectors
#ifdef FORENSIC_RBA_UNITS
            le32(c + 40, biosAt * 4);                          // (the forensic lab's build)
#else
            le32(c + 40, biosAt);                              // load RBA: in 2048-byte CD sectors
#endif
            std::copy(bios.begin(), bios.end(), sec(biosAt));
            if (!efi.empty()) {
                c[64] = 0x91;                                  // final section header
                c[65] = 0xEF;                                  // platform: EFI
                le16(c + 66, 1);
                c[96] = 0x88;
                le16(c + 102, static_cast<std::uint32_t>(std::min<std::size_t>(efi.size() / 512, 0xFFFF)));
                le32(c + 104, efiAt);
                std::copy(efi.begin(), efi.end(), sec(efiAt));
            }
        }
        std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(argv[1], "wb"), &std::fclose);
        if (!f || std::fwrite(img.data(), 1, img.size(), f.get()) != img.size()) throw std::runtime_error("cannot write image");
        std::printf("%s: %u sectors; directories %zu; path tables at %u/%u (Joliet %u/%u); catalog %u; BIOS image %u; EFI image %u\n",
                    argv[1], total, iso.dirs.size(), lP, mP, lJ, mJ, catalog, bios.empty() ? 0 : biosAt, efi.empty() ? 0 : efiAt);
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "mkiso: %s\n", e.what());
        return 1;
    }
}
