// isoread.cc - F3-48: a read-only ISO 9660 reader with Rock Ridge and Joliet, and an
// El Torito boot-catalog dumper (milestone FS5). It shares no code with mkiso.cc.
//   isoread info <iso>                         volume descriptors
//   isoread ls <iso> rr|joliet|plain           every file, recursively (see the chapter)
//   isoread cat <iso> <path>                   a file's bytes (Rock Ridge names)
//   isoread boot <iso>                         the El Torito boot catalog
//   isoread extract <iso> <entry> <out>        write the image of boot entry n to <out>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Bytes = std::vector<std::uint8_t>;
constexpr std::uint32_t kSector = 2048;
std::uint32_t le16(const std::uint8_t* p) { return static_cast<std::uint32_t>(p[0] | p[1] << 8); }
std::uint32_t le32(const std::uint8_t* p) { return le16(p) | le16(p + 2) << 16; }
std::uint32_t be32(const std::uint8_t* p) { return static_cast<std::uint32_t>(p[0]) << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

std::uint32_t crc32(const Bytes& d)
{
    std::uint32_t c = 0xFFFFFFFFu;
    for (std::uint8_t b : d) {
        c ^= b;
        for (int k = 0; k < 8; ++k) c = (c & 1) ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return ~c;
}

std::string utf8FromUcs2be(const std::uint8_t* p, std::size_t n)
{
    std::string out;
    for (std::size_t i = 0; i + 1 < n; i += 2) {
        const std::uint32_t c = static_cast<std::uint32_t>(p[i] << 8 | p[i + 1]);
        if (c < 0x80) out += static_cast<char>(c);
        else if (c < 0x800) { out += static_cast<char>(0xC0 | c >> 6); out += static_cast<char>(0x80 | (c & 0x3F)); }
        else { out += static_cast<char>(0xE0 | c >> 12); out += static_cast<char>(0x80 | ((c >> 6) & 0x3F)); out += static_cast<char>(0x80 | (c & 0x3F)); }
    }
    return out;
}

struct Entry
{
    std::string name, link;
    std::uint32_t extent = 0, size = 0, mode = 0;
    bool dir = false, hasPx = false;
};

class Iso
{
public:
    explicit Iso(const std::string& path) : f_(std::fopen(path.c_str(), "rb"), &std::fclose)
    {
        if (!f_) throw std::runtime_error("cannot open " + path);
        for (std::uint32_t s = 16;; ++s) {                     // the volume descriptor set
            Bytes v = sector(s);
            if (std::string(&v[1], &v[6]) != "CD001") throw std::runtime_error("not an ISO 9660 image");
            descriptors_.push_back(v);
            if (v[0] == 1) pvd_ = v;
            if (v[0] == 2 && v[88] == '%' && v[89] == '/' && (v[90] == '@' || v[90] == 'C' || v[90] == 'E')) joliet_ = v;
            if (v[0] == 0 && std::string(&v[7], &v[7] + 23) == "EL TORITO SPECIFICATION") catalog_ = le32(&v[0x47]);
            if (v[0] == 255) break;
        }
        if (pvd_.empty()) throw std::runtime_error("no primary volume descriptor");
    }

    void info() const
    {
        for (const Bytes& v : descriptors_) {
            const char* kind = v[0] == 0 ? "boot record" : v[0] == 1 ? "primary" : v[0] == 2 ? "supplementary" : v[0] == 255 ? "terminator" : "other";
            std::printf("descriptor type %3u (%s)", v[0], kind);
            if (v[0] == 1) std::printf(": volume id \"%s\", %u sectors of %u bytes, path table %u bytes at L %u / M %u",
                                       trim(std::string(&v[40], &v[72])).c_str(), le32(&v[80]), le16(&v[128]), le32(&v[132]), le32(&v[140]), be32(&v[148]));
            if (v[0] == 2) std::printf(": escape \"%c%c%c\" (Joliet), volume id \"%s\"", v[88], v[89], v[90],
                                       trim(utf8FromUcs2be(&v[40], 32)).c_str());
            if (v[0] == 0) std::printf(": \"%s\", boot catalog at sector %u", trim(std::string(&v[7], &v[39])).c_str(), le32(&v[0x47]));
            std::printf("\n");
        }
    }

    // Directory records of a directory extent; with Rock Ridge, NM/PX/SL replace the ISO names.
    std::vector<Entry> list(std::uint32_t extent, std::uint32_t size, bool rr, bool jol)
    {
        std::vector<Entry> out;
        for (std::uint32_t s = 0; s < size / kSector; ++s) {
            Bytes b = sector(extent + s);
            for (std::uint32_t off = 0; off < kSector && b[off] != 0; off += b[off]) {
                const std::uint8_t* r = &b[off];
                const std::uint32_t idLen = r[32];
                if (idLen == 1 && (r[33] == 0 || r[33] == 1)) continue;   // "." and ".."
                Entry e;
                e.extent = le32(r + 2);
                e.size = le32(r + 10);
                e.dir = (r[25] & 0x02) != 0;
                if (jol) e.name = utf8FromUcs2be(r + 33, idLen);
                else e.name = std::string(r + 33, r + 33 + idLen);
                if (!e.dir && e.name.size() > 2 && e.name.compare(e.name.size() - 2, 2, ";1") == 0) e.name.resize(e.name.size() - 2);
                if (rr) susp(r + 33 + idLen + (idLen % 2 == 0 ? 1 : 0), r + r[0], e);
                out.push_back(e);
            }
        }
        return out;
    }
    Entry root(bool jol) const
    {
        const std::uint8_t* r = &(jol ? joliet_ : pvd_)[156];
        Entry e;
        e.extent = le32(r + 2);
        e.size = le32(r + 10);
        e.dir = true;
        return e;
    }
    bool hasJoliet() const { return !joliet_.empty(); }
    Bytes read(const Entry& e)
    {
        Bytes d;
        for (std::uint32_t s = 0; s * kSector < e.size; ++s) {
            Bytes b = sector(e.extent + s);
            d.insert(d.end(), b.begin(), b.end());
        }
        d.resize(e.size);
        return d;
    }
    Entry find(const std::string& path)
    {
        Entry cur = root(false);
        for (std::size_t pos = 1; pos < path.size();) {
            std::size_t next = path.find('/', pos);
            if (next == std::string::npos) next = path.size();
            bool found = false;
            for (const Entry& e : list(cur.extent, cur.size, true, false)) {
                if (e.name == path.substr(pos, next - pos)) { cur = e; found = true; }
            }
            if (!found) throw std::runtime_error("no such file: " + path);
            pos = next + 1;
        }
        return cur;
    }

    struct BootEntry { std::uint32_t platform, media, segment, count, rba; bool bootable; };
    std::vector<BootEntry> boot(bool print)
    {
        if (!catalog_) throw std::runtime_error("no El Torito boot record");
        Bytes c = sector(catalog_);
        std::uint32_t sum = 0;
        for (int i = 0; i < 32; i += 2) sum += le16(&c[i]);
        const bool valid = c[0] == 1 && c[30] == 0x55 && c[31] == 0xAA && (sum & 0xFFFF) == 0;
        if (print) std::printf("validation entry: header 0x%02x, platform 0x%02x, id \"%s\", checksum word 0x%04x, key %02x %02x, words sum to 0x%04x: %s\n",
                               c[0], c[1], trim(std::string(&c[4], &c[28])).c_str(), le16(&c[28]), c[30], c[31], sum & 0xFFFF,
                               valid ? "VALID" : "INVALID");
        std::vector<BootEntry> out;
        std::uint32_t platform = c[1];
        auto entry = [&](const std::uint8_t* e, const char* what) {
            BootEntry b{platform, e[1], le16(e + 2), le16(e + 6), le32(e + 8), e[0] == 0x88};
            out.push_back(b);
            if (print) std::printf("%s: %s, platform 0x%02x (%s), media %u (%s), load segment 0x%04x, %u virtual sectors, image at sector %u\n",
                                   what, b.bootable ? "bootable" : "not bootable", b.platform,
                                   b.platform == 0 ? "80x86 BIOS" : b.platform == 0xEF ? "UEFI" : "other", b.media,
                                   b.media == 0 ? "no emulation" : "emulation", b.segment, b.count, b.rba);
        };
        entry(&c[32], "default entry");
        for (std::uint32_t off = 64; off + 32 <= kSector && (c[off] == 0x90 || c[off] == 0x91);) {
            platform = c[off + 1];
            const std::uint32_t n = le16(&c[off + 2]);
            const bool last = c[off] == 0x91;
            off += 32;
            for (std::uint32_t i = 0; i < n; ++i, off += 32) entry(&c[off], "section entry");
            if (last) break;
        }
        return out;
    }

private:
    Bytes sector(std::uint32_t s)
    {
        Bytes b(kSector);
        if (std::fseek(f_.get(), static_cast<long>(s) * kSector, SEEK_SET) != 0 || std::fread(b.data(), 1, kSector, f_.get()) != kSector)
            throw std::runtime_error("read beyond the end of the image");
        return b;
    }
    static std::string trim(std::string s)
    {
        while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) s.pop_back();
        return s;
    }
    static void susp(const std::uint8_t* p, const std::uint8_t* end, Entry& e)
    {
        std::string nm;
        bool haveNm = false;
        while (p + 4 <= end && p[2] >= 4 && p + p[2] <= end) {
            const std::string sig(p, p + 2);
            const std::uint32_t len = p[2];
            if (sig == "PX") { e.mode = le32(p + 4); e.hasPx = true; }
            if (sig == "NM") { nm.append(p + 5, p + len); haveNm = true; }
            if (sig == "SL") {
                for (const std::uint8_t* c = p + 5; c + 2 <= p + len; c += 2 + c[1]) {
                    if (!e.link.empty() && e.link.back() != '/') e.link += '/';
                    if (c[0] & 0x08) e.link = "/";
                    else if (c[0] & 0x02) e.link += ".";
                    else if (c[0] & 0x04) e.link += "..";
                    else e.link.append(c + 2, c + 2 + c[1]);
                }
            }
            p += len;
        }
        if (haveNm) e.name = nm;
    }

    std::unique_ptr<FILE, int (*)(FILE*)> f_;
    std::vector<Bytes> descriptors_;
    Bytes pvd_, joliet_;
    std::uint32_t catalog_ = 0;
};

void walk(Iso& iso, const Entry& dir, const std::string& path, bool rr, bool jol)
{
    for (const Entry& e : iso.list(dir.extent, dir.size, rr, jol)) {
        const std::string p = path + "/" + e.name;
        if (e.dir) {
            if (rr) std::printf("d %s %o\n", p.c_str(), e.mode);
            else std::printf("d %s\n", p.c_str());
            walk(iso, e, p, rr, jol);
        } else if (!e.link.empty()) {
            std::printf("l %s -> %s\n", p.c_str(), e.link.c_str());
        } else {
            const Bytes d = iso.read(e);
            if (rr) std::printf("f %s %o %zu %08x\n", p.c_str(), e.mode, d.size(), crc32(d));
            else std::printf("f %s %zu %08x\n", p.c_str(), d.size(), crc32(d));
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const std::vector<std::string> a(argv + 1, argv + argc);
        if (a.size() < 2) throw std::runtime_error("usage: see the comment at the top of isoread.cc");
        Iso iso(a[1]);
        if (a[0] == "info") {
            iso.info();
        } else if (a[0] == "ls" && a.size() == 3) {
            const bool jol = a[2] == "joliet", rr = a[2] == "rr";
            if (jol && !iso.hasJoliet()) throw std::runtime_error("no Joliet volume descriptor");
            walk(iso, iso.root(jol), "", rr, jol);
        } else if (a[0] == "cat" && a.size() == 3) {
            const Bytes d = iso.read(iso.find(a[2]));
            std::fwrite(d.data(), 1, d.size(), stdout);
        } else if (a[0] == "boot") {
            iso.boot(true);
        } else if (a[0] == "extract" && a.size() == 4) {
            const auto entries = iso.boot(false);
            const auto& b = entries.at(std::stoul(a[2]));
            Entry e;
            e.extent = b.rba;
            e.size = b.count * 512;
            const Bytes d = iso.read(e);
            std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(a[3].c_str(), "wb"), &std::fclose);
            if (!f || std::fwrite(d.data(), 1, d.size(), f.get()) != d.size()) throw std::runtime_error("cannot write " + a[3]);
            std::printf("wrote %zu bytes from sector %u to %s\n", d.size(), b.rba, a[3].c_str());
        } else {
            throw std::runtime_error("usage: see the comment at the top of isoread.cc");
        }
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "isoread: %s\n", e.what());
        return 1;
    }
}
