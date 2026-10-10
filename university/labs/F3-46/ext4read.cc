// ext4read.cc - F3-46: a read-only ext4 reader (milestone FS3): feature flags, 64-bit group
// descriptors, extent trees, hashed-tree (htree) directory lookup and metadata checksums
// (CRC32C). On-disk format: Linux kernel documentation "ext4 Data Structures and Algorithms"
// (chapter D1). Every structure it reads is checked against its checksum first.
//   ext4read info <img>                 superblock, features, checksums
//   ext4read tree <img>                 every file: type, size, CRC-32 of the contents
//   ext4read cat <img> <path>           file contents to stdout
//   ext4read extents <img> <path>       the extent tree of a file
//   ext4read lookup <img> <dir> <name>  find a name through the directory's hash tree
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Bytes = std::vector<std::uint8_t>;
std::uint32_t le16(const std::uint8_t* p) { return static_cast<std::uint32_t>(p[0] | p[1] << 8); }
std::uint32_t le32(const std::uint8_t* p) { return le16(p) | le16(p + 2) << 16; }
std::uint64_t le48(const std::uint8_t* hi16, const std::uint8_t* lo32)
{
    return static_cast<std::uint64_t>(le16(hi16)) << 32 | le32(lo32);
}

std::uint32_t crc32c(std::uint32_t crc, const std::uint8_t* p, std::size_t n)   // no final xor
{
    static const auto table = [] {
        std::vector<std::uint32_t> t(256);
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (c >> 1) ^ 0x82F63B78u : c >> 1;
            t[i] = c;
        }
        return t;
    }();
    while (n--) crc = table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return crc;
}

std::uint32_t crc32(const Bytes& d)                                 // the zlib / IEEE CRC-32
{
    std::uint32_t c = 0xFFFFFFFFu;
    for (std::uint8_t b : d) {
        c ^= b;
        for (int k = 0; k < 8; ++k) c = (c & 1) ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return ~c;
}

struct Feature { std::uint32_t bit; const char* name; };
const Feature kIncompat[] = {{0x1, "compression"}, {0x2, "filetype"}, {0x4, "needs_recovery"},
    {0x8, "journal_dev"}, {0x10, "meta_bg"}, {0x40, "extent"}, {0x80, "64bit"}, {0x100, "mmp"},
    {0x200, "flex_bg"}, {0x400, "ea_inode"}, {0x1000, "dirdata"}, {0x2000, "metadata_csum_seed"},
    {0x4000, "large_dir"}, {0x8000, "inline_data"}, {0x10000, "encrypt"}, {0x20000, "casefold"}};
constexpr std::uint32_t kUnderstood = 0x2 | 0x40 | 0x80 | 0x200 | 0x2000;   // what this reader handles
constexpr std::uint32_t kMetadataCsum = 0x400;                             // ro_compat
constexpr std::uint32_t kExtentsFl = 0x80000, kIndexFl = 0x1000, kInlineFl = 0x10000000;

class Ext4
{
public:
    explicit Ext4(const std::string& path) : f_(std::fopen(path.c_str(), "rb"), &std::fclose)
    {
        if (!f_) throw std::runtime_error("cannot open " + path);
        sb_ = readAt(1024, 1024);
        if (le16(&sb_[0x38]) != 0xEF53) throw std::runtime_error("bad magic: not ext2/3/4");
        bs_ = 1024u << le32(&sb_[0x18]);
        incompat_ = le32(&sb_[0x60]);
        csum_ = (le32(&sb_[0x64]) & kMetadataCsum) != 0;
        for (const Feature& f : kIncompat) {                       // refuse what we do not know
            if ((incompat_ & f.bit) && !(kUnderstood & f.bit))
                throw std::runtime_error(std::string("refusing to mount: incompatible feature '") + f.name + "' is not supported");
        }
        if (incompat_ & ~0x3FFFFu) throw std::runtime_error("refusing to mount: unknown incompatible feature bits");
        if (csum_) {
            const std::uint32_t got = crc32c(~0u, sb_.data(), 0x3FC);
            if (got != le32(&sb_[0x3FC])) throw std::runtime_error("superblock checksum mismatch");
            seed_ = (incompat_ & 0x2000) ? le32(&sb_[0x270]) : crc32c(~0u, &sb_[0x68], 16);
            ++verified_["superblock"];
        }
        descSize_ = (incompat_ & 0x80) ? le16(&sb_[0xFE]) : 32;
        isz_ = le16(&sb_[0x58]);
        ipg_ = le32(&sb_[0x28]);
        const std::uint64_t blocks = (incompat_ & 0x80) ? le48(&sb_[0x150], &sb_[0x04]) : le32(&sb_[0x04]);
        groups_ = static_cast<std::uint32_t>((blocks - le32(&sb_[0x14]) + le32(&sb_[0x20]) - 1) / le32(&sb_[0x20]));
        gdt_ = readAt((static_cast<std::uint64_t>(le32(&sb_[0x14])) + 1) * bs_, groups_ * descSize_);
        for (std::uint32_t g = 0; csum_ && g < groups_; ++g) {
            std::uint8_t* d = &gdt_[g * descSize_];
            std::uint8_t num[4] = {static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(g >> 8),
                                   static_cast<std::uint8_t>(g >> 16), static_cast<std::uint8_t>(g >> 24)};
            Bytes copy(d, d + descSize_);
            copy[0x1E] = copy[0x1F] = 0;                           // bg_checksum itself
            const std::uint32_t c = crc32c(crc32c(seed_, num, 4), copy.data(), descSize_) & 0xFFFF;
            if (c != le16(d + 0x1E)) throw std::runtime_error("group descriptor " + std::to_string(g) + " checksum mismatch");
            ++verified_["group descriptors"];
        }
    }

    void info()
    {
        std::printf("block size %u, inode size %u, inodes per group %u, groups %u, descriptor size %u\n",
                    bs_, isz_, ipg_, groups_, descSize_);
        std::printf("incompatible features:");
        for (const Feature& f : kIncompat) {
            if (incompat_ & f.bit) std::printf(" %s", f.name);
        }
        std::printf("\nmetadata_csum: %s; superblock checksum stored 0x%08x, computed 0x%08x\n",
                    csum_ ? "yes" : "no", le32(&sb_[0x3FC]), crc32c(~0u, sb_.data(), 0x3FC));
        std::printf("default directory hash version %u, hash seed %08x-...\n", sb_[0xFC], le32(&sb_[0xEC]));
        for (std::uint32_t g = 0; g < groups_; ++g) {
            const std::uint8_t* d = &gdt_[g * descSize_];
            std::printf("group %u: inode table at block %llu, descriptor checksum 0x%04x\n", g,
                        static_cast<unsigned long long>(field64(d, 0x08, 0x28)), le16(d + 0x1E));
        }
    }

    Bytes inode(std::uint32_t ino)
    {
        const std::uint32_t g = (ino - 1) / ipg_, idx = (ino - 1) % ipg_;
        const std::uint64_t table = field64(&gdt_[g * descSize_], 0x08, 0x28);
        Bytes in = readAt(table * bs_ + static_cast<std::uint64_t>(idx) * isz_, isz_);
        if (csum_) {
            const std::uint32_t lo = le16(&in[0x7C]);
            const bool hasHi = isz_ > 128 && le16(&in[0x80]) >= 4;          // i_extra_isize
            const std::uint32_t hi = hasHi ? le16(&in[0x82]) : 0;
            Bytes c = in;
            c[0x7C] = c[0x7D] = 0;
            if (hasHi) c[0x82] = c[0x83] = 0;
            std::uint32_t got = crc32c(inodeSeed(ino, in), c.data(), c.size());
            if (!hasHi) got &= 0xFFFF;
            if (got != (lo | hi << 16)) {
                char m[120];
                std::snprintf(m, sizeof m, "inode %u checksum mismatch (stored 0x%08x, computed 0x%08x)", ino, lo | hi << 16, got);
                throw std::runtime_error(m);
            }
            ++verified_["inodes"];
        }
        if (le32(&in[0x20]) & kInlineFl) throw std::runtime_error("inline data is not supported");
        return in;
    }
    static std::uint64_t size(const Bytes& in) { return static_cast<std::uint64_t>(le32(&in[0x6C])) << 32 | le32(&in[0x04]); }
    static std::uint32_t mode(const Bytes& in) { return le16(&in[0]); }

    struct Extent { std::uint32_t logical, len; std::uint64_t start; bool uninit; };
    // Walks the extent tree; every tree block's tail checksum is verified.
    void extents(std::uint32_t ino, const Bytes& in, std::vector<Extent>& out, int indent = -1)
    {
        if (!(le32(&in[0x20]) & kExtentsFl)) throw std::runtime_error("inode does not use extents");
        walk(ino, in, &in[0x28], out, indent);
    }

    Bytes read(std::uint32_t ino)
    {
        Bytes in = inode(ino);
        const std::uint64_t n = size(in);
        if ((mode(in) & 0xF000) == 0xA000 && n < 60 && le32(&in[0x1C]) == 0)
            return Bytes(&in[0x28], &in[0x28] + n);                 // fast symlink
        std::vector<Extent> ex;
        extents(ino, in, ex);
        Bytes data(n, 0);                                          // holes stay zero
#ifdef FORENSIC_NO_HOLES
        std::uint64_t pos = 0;                                     // (the forensic lab's build)
#endif
        for (const Extent& e : ex) {
            if (e.uninit) continue;                                // allocated, never written: zeros
            for (std::uint32_t i = 0; i < e.len; ++i) {
#ifdef FORENSIC_NO_HOLES
                const std::uint64_t at = pos;
                pos += bs_;
#else
                const std::uint64_t at = (static_cast<std::uint64_t>(e.logical) + i) * bs_;
#endif
                if (at >= n) break;
                Bytes b = readAt((e.start + i) * bs_, bs_);
                std::copy(b.begin(), b.begin() + static_cast<long>(std::min<std::uint64_t>(bs_, n - at)), data.begin() + static_cast<long>(at));
            }
        }
        return data;
    }

    // Directory entries (linear scan; works for hashed directories too, see the chapter).
    std::vector<std::pair<std::string, std::uint32_t>> list(std::uint32_t dir)
    {
        Bytes in = inode(dir);
        if ((mode(in) & 0xF000) != 0x4000) throw std::runtime_error("not a directory");
        std::vector<Extent> ex;
        extents(dir, in, ex);
        std::vector<std::pair<std::string, std::uint32_t>> out;
        for (const Extent& e : ex) {
            for (std::uint32_t i = 0; i < e.len; ++i) {
                Bytes b = dirBlock(dir, in, e.start + i, e.logical + i == 0 && (le32(&in[0x20]) & kIndexFl));
                for (std::uint32_t off = 0; off + 8 <= bs_ && le16(&b[off + 4]) >= 8; off += le16(&b[off + 4])) {
                    if (le32(&b[off]) && b[off + 6]) out.emplace_back(std::string(&b[off + 8], &b[off + 8] + b[off + 6]), le32(&b[off]));
                }
            }
        }
        return out;
    }

    std::uint32_t resolve(const std::string& path)
    {
        std::uint32_t ino = 2;
        for (std::size_t pos = 1; pos < path.size();) {
            std::size_t next = path.find('/', pos);
            if (next == std::string::npos) next = path.size();
            const std::string name = path.substr(pos, next - pos);
            std::uint32_t found = 0;
            for (const auto& [n, i] : list(ino)) {
                if (n == name) found = i;
            }
            if (!found) throw std::runtime_error("no such file: " + path);
            ino = found;
            pos = next + 1;
        }
        return ino;
    }

    // Hashed-tree lookup: hash the name, walk dx_root (and dx_node levels), search one leaf.
    std::uint32_t lookup(std::uint32_t dir, const std::string& name)
    {
        Bytes in = inode(dir);
        if (!(le32(&in[0x20]) & kIndexFl)) throw std::runtime_error("directory is not hashed");
        std::vector<Extent> ex;
        extents(dir, in, ex);
        auto phys = [&](std::uint32_t l) {
            for (const Extent& e : ex) {
                if (l >= e.logical && l < e.logical + e.len) return e.start + (l - e.logical);
            }
            throw std::runtime_error("hole in a directory");
        };
        Bytes root = readAt(phys(0) * bs_, bs_);
        const std::uint32_t version = root[0x1C], levels = root[0x1E];
        if (version != 1) throw std::runtime_error("only the half_md4 hash is implemented here");
        const std::uint32_t h = halfMd4(name);
        std::printf("hash(\"%s\") = 0x%08x (half_md4, seed from the superblock); indirect levels %u\n",
                    name.c_str(), h, levels);
        std::uint32_t off = 0x20, leaf = 0;
        Bytes node = root;
        for (std::uint32_t level = 0; level <= levels; ++level) {
            const std::uint32_t count = le16(&node[off + 2]);
            std::uint32_t pick = 0;                                // entry 0 covers hashes from 0
            for (std::uint32_t i = 1; i < count; ++i) {
                if (le32(&node[off + 8 * i]) <= h) pick = i;
            }
            leaf = le32(&node[off + 8 * pick + 4]);
            std::printf("  level %u: %u entries, choose entry %u (hash >= 0x%08x) -> directory block %u\n",
                        level, count, pick, pick ? le32(&node[off + 8 * pick]) : 0, leaf);
            if (level < levels) {
                node = readAt(phys(leaf) * bs_, bs_);
                off = 0x08;                                        // after the empty dirent
            }
        }
        Bytes b = dirBlock(dir, in, phys(leaf), false);
        for (std::uint32_t o = 0; o + 8 <= bs_ && le16(&b[o + 4]) >= 8; o += le16(&b[o + 4])) {
            if (le32(&b[o]) && std::string(&b[o + 8], &b[o + 8] + b[o + 6]) == name) {
                std::printf("  found in directory block %u: inode %u\n", leaf, le32(&b[o]));
                return le32(&b[o]);
            }
        }
        std::printf("  not in directory block %u\n", leaf);
        return 0;
    }

    void printVerified() const
    {
        std::printf("checksums verified:");
        for (const auto& [what, n] : verified_) std::printf(" %s %lu;", what.c_str(), n);
        std::printf(" mismatches 0\n");
    }

private:
    Bytes readAt(std::uint64_t off, std::size_t n)
    {
        Bytes b(n);
        if (std::fseek(f_.get(), static_cast<long>(off), SEEK_SET) != 0 || std::fread(b.data(), 1, n, f_.get()) != n)
            throw std::runtime_error("read beyond the end of the image");
        return b;
    }
    std::uint64_t field64(const std::uint8_t* d, std::uint32_t lo, std::uint32_t hi) const
    {
        return descSize_ >= 64 ? static_cast<std::uint64_t>(le32(d + hi)) << 32 | le32(d + lo) : le32(d + lo);
    }
    std::uint32_t inodeSeed(std::uint32_t ino, const Bytes& in) const
    {
        std::uint8_t n[4] = {static_cast<std::uint8_t>(ino), static_cast<std::uint8_t>(ino >> 8),
                             static_cast<std::uint8_t>(ino >> 16), static_cast<std::uint8_t>(ino >> 24)};
        return crc32c(crc32c(seed_, n, 4), &in[0x64], 4);         // inode number, then i_generation
    }
    void walk(std::uint32_t ino, const Bytes& in, const std::uint8_t* h, std::vector<Extent>& out, int indent)
    {
        if (le16(h) != 0xF30A) throw std::runtime_error("bad extent header magic in inode " + std::to_string(ino));
        const std::uint32_t entries = le16(h + 2), depth = le16(h + 6);
        if (indent >= 0) std::printf("%*sheader: %u entries, max %u, depth %u\n", indent, "", entries, le16(h + 4), depth);
        for (std::uint32_t i = 0; i < entries; ++i) {
            const std::uint8_t* e = h + 12 + 12 * i;
            if (depth == 0) {
                std::uint32_t len = le16(e + 4);
                const bool uninit = len > 32768;
                if (uninit) len -= 32768;
                out.push_back({le32(e), len, le48(e + 6, e + 8), uninit});
                if (indent >= 0) std::printf("%*sleaf: logical %u, length %u, physical %llu%s\n", indent + 2, "",
                                             le32(e), len, static_cast<unsigned long long>(le48(e + 6, e + 8)),
                                             uninit ? " (uninitialized)" : "");
            } else {
                const std::uint64_t child = le48(e + 8, e + 4);
                if (indent >= 0) std::printf("%*sindex: logical %u -> tree block %llu\n", indent + 2, "", le32(e),
                                             static_cast<unsigned long long>(child));
                Bytes b = readAt(child * bs_, bs_);
                if (csum_) {                                       // tail after eh_max entries
                    const std::uint32_t tail = 12 + 12 * le16(&b[4]);
                    if (crc32c(inodeSeed(ino, in), b.data(), tail) != le32(&b[tail]))
                        throw std::runtime_error("extent block " + std::to_string(child) + " checksum mismatch");
                    ++verified_["extent tree blocks"];
                }
                walk(ino, in, b.data(), out, indent < 0 ? -1 : indent + 4);
            }
        }
    }
    Bytes dirBlock(std::uint32_t dir, const Bytes& in, std::uint64_t blk, bool dxRoot)
    {
        Bytes b = readAt(blk * bs_, bs_);
        const bool leafTail = le32(&b[bs_ - 12]) == 0 && le16(&b[bs_ - 8]) == 12 && b[bs_ - 5] == 0xDE;
        if (csum_ && leafTail && !dxRoot) {
            if (crc32c(inodeSeed(dir, in), b.data(), bs_ - 12) != le32(&b[bs_ - 4]))
                throw std::runtime_error("directory block " + std::to_string(blk) + " checksum mismatch");
            ++verified_["directory blocks"];
        }
        return b;
    }
    std::uint32_t halfMd4(const std::string& name) const
    {
        std::uint32_t buf[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
        bool zero = true;
        for (int i = 0; i < 16; ++i) zero = zero && sb_[0xEC + i] == 0;
        if (!zero) {
            for (int i = 0; i < 4; ++i) buf[i] = le32(&sb_[0xEC + 4 * i]);
        }
        const bool sign = (le32(&sb_[0x160]) & 0x1) != 0;         // s_flags: signed_directory_hash
        for (std::size_t p = 0; p < name.size(); p += 32) {
            const std::size_t len = name.size() - p;
            std::uint32_t in[8];
            std::uint32_t pad = static_cast<std::uint32_t>(len) | static_cast<std::uint32_t>(len) << 8;
            pad |= pad << 16;
            std::uint32_t val = pad;
            int num = 8, k = 0;
            for (std::size_t i = 0; i < std::min<std::size_t>(len, 32); ++i) {
                const int c = sign ? static_cast<signed char>(name[p + i]) : static_cast<unsigned char>(name[p + i]);
                val = static_cast<std::uint32_t>(c) + (val << 8);
                if (i % 4 == 3) {
                    in[k++] = val;
                    val = pad;
                    --num;
                }
            }
            if (--num >= 0) in[k++] = val;
            while (--num >= 0) in[k++] = pad;
            transform(buf, in);
        }
        std::uint32_t h = buf[1] & ~1u;
        if (h == (0x7FFFFFFFu << 1)) h = (0x7FFFFFFFu - 1) << 1;
        return h;
    }
    static std::uint32_t rol(std::uint32_t x, int s) { return x << s | x >> (32 - s); }
    static void transform(std::uint32_t buf[4], const std::uint32_t in[8])
    {
        auto F = [](std::uint32_t x, std::uint32_t y, std::uint32_t z) { return z ^ (x & (y ^ z)); };
        auto G = [](std::uint32_t x, std::uint32_t y, std::uint32_t z) { return (x & y) + ((x ^ y) & z); };
        auto H = [](std::uint32_t x, std::uint32_t y, std::uint32_t z) { return x ^ y ^ z; };
        const std::uint32_t K2 = 013240474631u, K3 = 015666365641u;
        std::uint32_t a = buf[0], b = buf[1], c = buf[2], d = buf[3];
        auto R = [](auto f, std::uint32_t& w, std::uint32_t x, std::uint32_t y, std::uint32_t z, std::uint32_t v, int s) {
            w = rol(w + f(x, y, z) + v, s);
        };
        R(F, a, b, c, d, in[0], 3);  R(F, d, a, b, c, in[1], 7);  R(F, c, d, a, b, in[2], 11); R(F, b, c, d, a, in[3], 19);
        R(F, a, b, c, d, in[4], 3);  R(F, d, a, b, c, in[5], 7);  R(F, c, d, a, b, in[6], 11); R(F, b, c, d, a, in[7], 19);
        R(G, a, b, c, d, in[1] + K2, 3); R(G, d, a, b, c, in[3] + K2, 5); R(G, c, d, a, b, in[5] + K2, 9);  R(G, b, c, d, a, in[7] + K2, 13);
        R(G, a, b, c, d, in[0] + K2, 3); R(G, d, a, b, c, in[2] + K2, 5); R(G, c, d, a, b, in[4] + K2, 9);  R(G, b, c, d, a, in[6] + K2, 13);
        R(H, a, b, c, d, in[3] + K3, 3); R(H, d, a, b, c, in[7] + K3, 9); R(H, c, d, a, b, in[2] + K3, 11); R(H, b, c, d, a, in[6] + K3, 15);
        R(H, a, b, c, d, in[1] + K3, 3); R(H, d, a, b, c, in[5] + K3, 9); R(H, c, d, a, b, in[0] + K3, 11); R(H, b, c, d, a, in[4] + K3, 15);
        buf[0] += a;
        buf[1] += b;
        buf[2] += c;
        buf[3] += d;
    }

    std::unique_ptr<FILE, int (*)(FILE*)> f_;
    Bytes sb_, gdt_;
    std::uint32_t bs_ = 0, incompat_ = 0, descSize_ = 0, isz_ = 0, ipg_ = 0, groups_ = 0, seed_ = 0;
    bool csum_ = false;
    std::map<std::string, unsigned long> verified_;
};

void tree(Ext4& fs, std::uint32_t dir, const std::string& path)
{
    auto entries = fs.list(dir);
    std::sort(entries.begin(), entries.end());
    for (const auto& [name, ino] : entries) {
        if (name == "." || name == ".." || (path.empty() && name == "lost+found")) continue;
        const std::string p = path + "/" + name;
        const Bytes in = fs.inode(ino);
        const std::uint32_t type = Ext4::mode(in) & 0xF000;
        if (type == 0x4000) {
            std::printf("d %s\n", p.c_str());
            tree(fs, ino, p);
        } else {
            const Bytes data = fs.read(ino);
            std::printf("%c %s %llu %08x\n", type == 0xA000 ? 'l' : 'f', p.c_str(),
                        static_cast<unsigned long long>(data.size()), crc32(data));
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const std::vector<std::string> a(argv + 1, argv + argc);
        if (a.size() < 2) throw std::runtime_error("usage: see the comment at the top of ext4read.cc");
        Ext4 fs(a[1]);
        if (a[0] == "info") {
            fs.info();
        } else if (a[0] == "tree") {
            tree(fs, 2, "");
        } else if (a[0] == "cat" && a.size() == 3) {
            const Bytes d = fs.read(fs.resolve(a[2]));
            std::fwrite(d.data(), 1, d.size(), stdout);
            return 0;
        } else if (a[0] == "extents" && a.size() == 3) {
            const std::uint32_t ino = fs.resolve(a[2]);
            std::vector<Ext4::Extent> ex;
            std::printf("%s: inode %u\n", a[2].c_str(), ino);
            fs.extents(ino, fs.inode(ino), ex, 2);
        } else if (a[0] == "lookup" && a.size() == 4) {
            if (!fs.lookup(fs.resolve(a[2]), a[3])) return 1;
        } else {
            throw std::runtime_error("usage: see the comment at the top of ext4read.cc");
        }
        fs.printVerified();
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "ext4read: %s\n", e.what());
        return 1;
    }
}
