// ext2w.h - F3-44: the write paths of a small ext2 driver (create, write with direct, single
// and double indirect blocks, sparse files, truncate, unlink, rename, hard and symbolic links,
// mkdir) with careful write ordering. All block traffic goes through a Store, so F3-45 can
// put a journal underneath without touching this file.
// Layout facts: Poirier, "The Second Extended File System: Internal Layout" (chapter D1).
#pragma once
#include "blockdev.h"
#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace os401 {

constexpr std::uint32_t kFixedTime = 1700000000;     // every timestamp: reproducible images
constexpr std::uint16_t kModeReg = 0x81A4;           // regular file, rw-r--r--
constexpr std::uint16_t kModeDir = 0x41ED;           // directory, rwxr-xr-x
constexpr std::uint16_t kModeLnk = 0xA1FF;           // symbolic link, rwxrwxrwx
constexpr std::uint8_t kFtReg = 1, kFtDir = 2, kFtLnk = 7;   // directory-entry file types
constexpr std::uint32_t kIndexFl = 0x1000;           // directory has a hashed index

// Where block traffic goes. order() means: everything written so far must be on the medium
// before anything written later (a cache flush between the two groups of writes).
class Store
{
public:
    virtual ~Store() = default;
    virtual void read(std::uint32_t blk, std::uint8_t* out) = 0;
    virtual void meta(std::uint32_t blk, const std::uint8_t* in) = 0;   // file-system metadata
    virtual void data(std::uint32_t blk, const std::uint8_t* in) = 0;   // file contents
    virtual void order() = 0;
    virtual void sync() = 0;                                            // fsync
};

class DirectStore : public Store
{
public:
    DirectStore(BlockDev& dev, bool careful) : dev_(dev), careful_(careful) {}
    void read(std::uint32_t b, std::uint8_t* out) override { dev_.read(b, out); }
    void meta(std::uint32_t b, const std::uint8_t* in) override { dev_.write(b, in); }
    void data(std::uint32_t b, const std::uint8_t* in) override { dev_.write(b, in); }
    void order() override { if (careful_) dev_.flush(); }
    void sync() override { dev_.flush(); }
private:
    BlockDev& dev_;
    bool careful_;
};

struct Inode
{
    std::uint32_t ino = 0;
    std::uint8_t raw[128] = {};      // the 128-byte ext2 inode; bytes we do not know are kept
    std::uint16_t mode() const { return get16(raw); }
    std::uint32_t size() const { return get32(raw + 4); }
    std::uint16_t links() const { return get16(raw + 26); }
    std::uint32_t sectors() const { return get32(raw + 28); }        // i_blocks, 512-byte units
    std::uint32_t flags() const { return get32(raw + 32); }
    std::uint32_t block(int i) const { return get32(raw + 40 + 4 * i); }
    void setSize(std::uint32_t v) { put32(raw + 4, v); }
    void setLinks(std::uint32_t v) { put16(raw + 26, v); }
    void setSectors(std::uint32_t v) { put32(raw + 28, v); }
    void setBlock(int i, std::uint32_t v) { put32(raw + 40 + 4 * i, v); }
    void setDtime(std::uint32_t v) { put32(raw + 20, v); }
    bool isDir() const { return (mode() & 0xF000) == 0x4000; }
    bool isFastLink() const { return (mode() & 0xF000) == 0xA000 && sectors() == 0; }
};

class Ext2
{
public:
    Ext2(Store& s, std::uint32_t blockSize, bool careless = false, std::uint32_t okIncompat = 0x2)
        : s_(s), bs_(blockSize), careless_(careless)
    {
        sbBlock_ = 1024 / bs_;
        sbOff_ = 1024 % bs_;
        Bytes b = blk(sbBlock_);
        sb_.assign(b.begin() + sbOff_, b.begin() + sbOff_ + 1024);
        if (get16(&sb_[56]) != 0xEF53) throw std::runtime_error("not ext2: bad magic");
        if ((1024u << get32(&sb_[24])) != bs_) throw std::runtime_error("block size mismatch");
        if (get32(&sb_[76]) < 1) throw std::runtime_error("revision 0 file system not supported");
        const std::uint32_t incompat = get32(&sb_[96]), rocompat = get32(&sb_[100]);
        if (incompat & ~okIncompat) throw std::runtime_error("unsupported incompatible features " + hex(incompat & ~okIncompat));
        if (rocompat & ~0x3u) throw std::runtime_error("read-only-compatible features " + hex(rocompat & ~0x3u) + ": refusing to write");
        firstData_ = get32(&sb_[20]);
        bpg_ = get32(&sb_[32]);
        ipg_ = get32(&sb_[40]);
        isz_ = get16(&sb_[88]);
        groups_ = (get32(&sb_[4]) - firstData_ + bpg_ - 1) / bpg_;
        gdtBlock_ = firstData_ + 1;
        for (std::uint32_t i = 0; i < (groups_ * 32 + bs_ - 1) / bs_; ++i) {
            Bytes g = blk(gdtBlock_ + i);
            gdt_.insert(gdt_.end(), g.begin(), g.end());
        }
    }

    std::uint32_t freeBlocks() const { return get32(&sb_[12]); }
    std::uint32_t superField32(std::size_t off) const { return get32(&sb_[off]); }
    // Device block of logical block l of inode ino (0 = hole). F3-45 maps the journal with it.
    std::uint32_t blockOf(std::uint32_t ino, std::uint32_t l) { return bmap(readInode(ino), l); }
    std::uint32_t sizeOf(std::uint32_t ino) { return readInode(ino).size(); }
    std::uint32_t freeInodes() const { return get32(&sb_[16]); }

    std::uint32_t resolve(const std::string& path)
    {
        std::uint32_t ino = 2;                                   // the root directory
        std::size_t pos = 1;
        while (ino && pos < path.size()) {
            std::size_t next = path.find('/', pos);
            if (next == std::string::npos) next = path.size();
            ino = lookup(ino, path.substr(pos, next - pos));
            pos = next + 1;
        }
        return ino;
    }

    std::uint32_t create(const std::string& path) { return makeNode(path, kModeReg, kFtReg, ""); }
    std::uint32_t symlink(const std::string& target, const std::string& path)
    {
        if (target.size() >= 60) throw std::runtime_error("only fast symlinks (< 60 bytes)");
        return makeNode(path, kModeLnk, kFtLnk, target);
    }

    std::uint32_t mkdir(const std::string& path)
    {
        auto [parent, name] = split(path);
        Inode p = readInode(parent);
        if (lookup(parent, name)) throw std::runtime_error("exists: " + path);
        const std::uint32_t ino = allocBit(true, group(parent), true);
        const std::uint32_t b = allocBit(false, group(ino));
        Bytes d(bs_, 0);                                          // "." and ".."
        putEntry(&d[0], ino, 12, ".", kFtDir);
        putEntry(&d[12], parent, bs_ - 12, "..", kFtDir);
        s_.meta(b, d.data());
        Inode in = freshInode(ino, kModeDir);
        in.setLinks(2);
        in.setSize(bs_);
        in.setSectors(bs_ / 512);
        in.setBlock(0, b);
        if (careless_) addEntry(p, name, ino, kFtDir);
        writeInode(in);
        s_.order();                                               // child complete before ...
        p = readInode(parent);
        p.setLinks(p.links() + 1u);                               // ... the parent counts it ...
        writeInode(p);
        s_.order();
        if (!careless_) addEntry(p, name, ino, kFtDir);           // ... and finally names it
        return ino;
    }

    void write(const std::string& path, std::uint32_t off, const Bytes& data)
    {
        if (data.empty()) return;
        Inode in = readInode(need(path));
        Pending pend;
        const std::uint32_t first = off / bs_, last = (off + static_cast<std::uint32_t>(data.size()) - 1) / bs_;
        for (std::uint32_t l = first; l <= last; ++l) {
            bool fresh = false;
            const std::uint32_t b = mapForWrite(in, l, pend, fresh);
            Bytes v = fresh ? Bytes(bs_, 0) : blk(b);
            const std::uint32_t lo = std::max(off, l * bs_);
            const std::uint32_t hi = std::min(off + static_cast<std::uint32_t>(data.size()), (l + 1) * bs_);
            std::copy(data.begin() + (lo - off), data.begin() + (hi - off), v.begin() + (lo - l * bs_));
            s_.data(b, v.data());                                 // 1. contents
        }
        for (auto& [b, ind] : pend) {
            if (ind.fresh) s_.meta(b, ind.bytes.data());          // 1. new indirect blocks
        }
        s_.order();
        in.setSize(std::max(in.size(), off + static_cast<std::uint32_t>(data.size())));
        in.setSectors(in.sectors() + pend.allocated * (bs_ / 512));
        writeInode(in);                                           // 2. the inode
        bool touched = false;
        for (auto& [b, ind] : pend) {
            if (!ind.fresh && ind.dirty) {
                if (!touched) s_.order();
                touched = true;
                s_.meta(b, ind.bytes.data());                     // 3. old indirect blocks
            }
        }
    }

    Bytes read(const std::string& path)
    {
        Inode in = readInode(need(path));
        Bytes out;
        for (std::uint32_t l = 0; l * bs_ < in.size(); ++l) {
            const std::uint32_t b = bmap(in, l);
            Bytes v = b ? blk(b) : Bytes(bs_, 0);                 // 0 = a hole: reads as zeros
            out.insert(out.end(), v.begin(), v.end());
        }
        out.resize(in.size());
        return out;
    }

    void truncate(const std::string& path, std::uint32_t size)
    {
        Inode in = readInode(need(path));
        if (size >= in.size()) return;
        const std::uint32_t keep = (size + bs_ - 1) / bs_, ppb = bs_ / 4;
        if (size % bs_) {                                         // zero the tail of the last block
            const std::uint32_t b = bmap(in, keep - 1);
            if (b) {
                Bytes v = blk(b);
                std::fill(v.begin() + size % bs_, v.end(), 0);
                s_.data(b, v.data());
            }
        }
        std::vector<std::uint32_t> gone;
        std::vector<std::pair<std::uint32_t, Bytes>> rewrites;    // kept indirect blocks
        for (std::uint32_t l = keep; l < 12; ++l) {
            if (in.block(static_cast<int>(l))) gone.push_back(in.block(static_cast<int>(l)));
            in.setBlock(static_cast<int>(l), 0);
        }
        if (std::uint32_t s1 = in.block(12)) {
            const std::uint32_t from = keep > 12 ? keep - 12 : 0;
            if (cutIndirect(s1, from, 1, gone, rewrites)) in.setBlock(12, 0);
        }
        if (std::uint32_t d = in.block(13)) {
            const std::uint32_t from = keep > 12 + ppb ? keep - 12 - ppb : 0;
            if (cutIndirect(d, from, 2, gone, rewrites)) in.setBlock(13, 0);
        }
        for (auto& [b, bytes] : rewrites) s_.meta(b, bytes.data());   // 1. drop pointers ...
        if (!rewrites.empty()) s_.order();
        in.setSize(size);
        in.setSectors(in.sectors() - static_cast<std::uint32_t>(gone.size()) * (bs_ / 512));
        writeInode(in);                                           // 2. ... and in the inode
        s_.order();
        for (std::uint32_t b : gone) freeBit(false, b);           // 3. only then free them
    }

    void unlink(const std::string& path)
    {
        auto [dir, name] = split(path);
        const std::uint32_t ino = lookup(dir, name);
        if (!ino) throw std::runtime_error("no such file: " + path);
        Inode in = readInode(ino);
        if (in.isDir()) throw std::runtime_error("unlink of a directory is not supported here");
        Inode d = readInode(dir);
        removeEntry(d, name);                                     // 1. the name goes first
        s_.order();
        in.setLinks(in.links() - 1u);
        if (in.links() > 0) {
            writeInode(in);
            return;
        }
        in.setDtime(kFixedTime);
        writeInode(in);                                           // 2. inode marked deleted
        s_.order();
        for (std::uint32_t b : allBlocks(in)) freeBit(false, b);  // 3. then space is freed
        freeBit(true, ino);
    }

    void link(const std::string& existing, const std::string& path)
    {
        Inode in = readInode(need(existing));
        if (in.isDir()) throw std::runtime_error("hard links to directories are not allowed");
        in.setLinks(in.links() + 1u);
        writeInode(in);                                           // count first ...
        s_.order();
        auto [dir, name] = split(path);
        Inode d = readInode(dir);
        addEntry(d, name, in.ino, fileType(in));                  // ... then the new name
    }

    void rename(const std::string& from, const std::string& to)
    {
        auto [odir, oname] = split(from);
        auto [ndir, nname] = split(to);
        Inode in = readInode(need(from));
        if (in.isDir()) throw std::runtime_error("renaming directories is not supported here");
        if (lookup(ndir, nname)) throw std::runtime_error("target exists: " + to);
        in.setLinks(in.links() + 1u);
        writeInode(in);                                           // 1. count the extra name
        s_.order();
        Inode nd = readInode(ndir);
        addEntry(nd, nname, in.ino, fileType(in));                // 2. new name
        s_.order();
        Inode od = readInode(odir);
        removeEntry(od, oname);                                   // 3. old name
        s_.order();
        in = readInode(in.ino);
        in.setLinks(in.links() - 1u);
        writeInode(in);                                           // 4. count back down
    }

    void fsync() { s_.sync(); }

private:
    struct IndBuf
    {
        Bytes bytes;
        bool fresh = false;
        bool dirty = false;
    };
    struct Pending : std::map<std::uint32_t, IndBuf>
    {
        std::uint32_t allocated = 0;
    };

    static std::string hex(std::uint32_t v)
    {
        char b[16];
        std::snprintf(b, sizeof b, "0x%x", v);
        return b;
    }
    Bytes blk(std::uint32_t b)
    {
        Bytes v(bs_);
        s_.read(b, v.data());
        return v;
    }
    std::uint32_t group(std::uint32_t ino) const { return (ino - 1) / ipg_; }
    std::uint8_t* gd(std::uint32_t g) { return &gdt_[g * 32]; }
    std::uint32_t blocksInGroup(std::uint32_t g) const
    {
        return std::min(bpg_, get32(&sb_[4]) - firstData_ - g * bpg_);
    }

    void writeSuper()
    {
        Bytes b = blk(sbBlock_);
        std::copy(sb_.begin(), sb_.end(), b.begin() + sbOff_);
        s_.meta(sbBlock_, b.data());
    }
    void writeGd(std::uint32_t g)
    {
        const std::uint32_t b = gdtBlock_ + g * 32 / bs_;
        Bytes v = blk(b);
        std::copy(gd(g), gd(g) + 32, v.begin() + (g * 32) % bs_);
        s_.meta(b, v.data());
    }

    // Allocation: bitmap bit, then the group's and the superblock's free counters.
    std::uint32_t allocBit(bool inode, std::uint32_t startGroup, bool dir = false)
    {
        for (std::uint32_t i = 0; i < groups_; ++i) {
            const std::uint32_t g = (startGroup + i) % groups_;
            const std::uint32_t freeCount = get16(gd(g) + (inode ? 14 : 12));
            if (freeCount == 0) continue;
            const std::uint32_t bmb = get32(gd(g) + (inode ? 4 : 0));
            Bytes bm = blk(bmb);
            const std::uint32_t n = inode ? ipg_ : blocksInGroup(g);
            for (std::uint32_t bit = 0; bit < n; ++bit) {
                if (bm[bit / 8] & (1u << (bit % 8))) continue;
                bm[bit / 8] = static_cast<std::uint8_t>(bm[bit / 8] | (1u << (bit % 8)));
                s_.meta(bmb, bm.data());
                put16(gd(g) + (inode ? 14 : 12), freeCount - 1);
                if (dir) put16(gd(g) + 16, get16(gd(g) + 16) + 1u);
                writeGd(g);
                put32(&sb_[inode ? 16 : 12], get32(&sb_[inode ? 16 : 12]) - 1);
                writeSuper();
                return inode ? g * ipg_ + bit + 1 : firstData_ + g * bpg_ + bit;
            }
        }
        throw std::runtime_error(inode ? "no free inodes" : "no free blocks");
    }
    void freeBit(bool inode, std::uint32_t n, bool dir = false)
    {
        const std::uint32_t idx = inode ? n - 1 : n - firstData_;
        const std::uint32_t g = idx / (inode ? ipg_ : bpg_), bit = idx % (inode ? ipg_ : bpg_);
        const std::uint32_t bmb = get32(gd(g) + (inode ? 4 : 0));
        Bytes bm = blk(bmb);
        bm[bit / 8] = static_cast<std::uint8_t>(bm[bit / 8] & ~(1u << (bit % 8)));
        s_.meta(bmb, bm.data());
        put16(gd(g) + (inode ? 14 : 12), get16(gd(g) + (inode ? 14 : 12)) + 1u);
        if (dir) put16(gd(g) + 16, get16(gd(g) + 16) - 1u);
        writeGd(g);
        put32(&sb_[inode ? 16 : 12], get32(&sb_[inode ? 16 : 12]) + 1);
        writeSuper();
    }

    std::pair<std::uint32_t, std::uint32_t> inodeLoc(std::uint32_t ino)
    {
        const std::uint32_t byte = ((ino - 1) % ipg_) * isz_;
        return {get32(gd(group(ino)) + 8) + byte / bs_, byte % bs_};
    }
    Inode readInode(std::uint32_t ino)
    {
        auto [b, o] = inodeLoc(ino);
        Bytes v = blk(b);
        Inode in;
        in.ino = ino;
        std::copy(v.begin() + o, v.begin() + o + 128, in.raw);
        return in;
    }
    void writeInode(const Inode& in)
    {
        auto [b, o] = inodeLoc(in.ino);
        Bytes v = blk(b);
        std::copy(in.raw, in.raw + 128, v.begin() + o);
        s_.meta(b, v.data());
    }
    Inode freshInode(std::uint32_t ino, std::uint16_t mode)
    {
        Inode in;
        in.ino = ino;
        put16(in.raw, mode);
        for (int off : {8, 12, 16}) put32(in.raw + off, kFixedTime);   // atime, ctime, mtime
        put32(in.raw + 100, ino);                                       // i_generation
        return in;
    }
    static std::uint8_t fileType(const Inode& in)
    {
        return in.isDir() ? kFtDir : ((in.mode() & 0xF000) == 0xA000 ? kFtLnk : kFtReg);
    }

    std::uint32_t makeNode(const std::string& path, std::uint16_t mode, std::uint8_t ft, const std::string& link)
    {
        auto [parent, name] = split(path);
        if (lookup(parent, name)) throw std::runtime_error("exists: " + path);
        Inode p = readInode(parent);
        const std::uint32_t ino = allocBit(true, group(parent));
        Inode in = freshInode(ino, mode);
        in.setLinks(1);
        if (!link.empty()) {
            std::copy(link.begin(), link.end(), in.raw + 40);     // fast symlink: in i_block
            in.setSize(static_cast<std::uint32_t>(link.size()));
        }
        if (careless_) addEntry(p, name, ino, ft);                // the bug of the forensic lab
        writeInode(in);
        s_.order();                                               // inode complete before ...
        if (!careless_) addEntry(p, name, ino, ft);               // ... a name points to it
        return ino;
    }

    std::uint32_t need(const std::string& path)
    {
        const std::uint32_t ino = resolve(path);
        if (!ino) throw std::runtime_error("no such file: " + path);
        return ino;
    }
    std::pair<std::uint32_t, std::string> split(const std::string& path)
    {
        const std::size_t cut = path.rfind('/');
        const std::uint32_t dir = cut == 0 ? 2 : resolve(path.substr(0, cut));
        if (!dir || !readInode(dir).isDir()) throw std::runtime_error("no such directory in " + path);
        return {dir, path.substr(cut + 1)};
    }

    // Block mapping: 12 direct pointers, one single and one double indirect block.
    std::uint32_t ptrIn(std::uint32_t blkNo, std::uint32_t idx) { return get32(&blk(blkNo)[idx * 4]); }
    std::uint32_t bmap(const Inode& in, std::uint32_t l)
    {
        const std::uint32_t ppb = bs_ / 4;
        if (l < 12) return in.block(static_cast<int>(l));
        l -= 12;
        if (l < ppb) return in.block(12) ? ptrIn(in.block(12), l) : 0;
        l -= ppb;
        if (l >= ppb * ppb) throw std::runtime_error("triple indirect blocks are not supported here");
        const std::uint32_t s = in.block(13) ? ptrIn(in.block(13), l / ppb) : 0;
        return s ? ptrIn(s, l % ppb) : 0;
    }
    IndBuf& ind(Pending& p, std::uint32_t b, bool fresh)
    {
        auto it = p.find(b);
        if (it != p.end()) return it->second;
        IndBuf& n = p[b];
        n.bytes = fresh ? Bytes(bs_, 0) : blk(b);
        n.fresh = fresh;
        return n;
    }
    std::uint32_t childFor(Pending& p, std::uint32_t parentBlk, std::uint32_t idx, bool isIndirect,
                           std::uint32_t goal, bool& fresh)
    {
        IndBuf& par = ind(p, parentBlk, false);
        std::uint32_t c = get32(&par.bytes[idx * 4]);
        fresh = false;
        if (!c) {
            c = allocBit(false, goal);
            ++p.allocated;
            put32(&par.bytes[idx * 4], c);                        // std::map keeps references valid
            par.dirty = true;
            if (isIndirect) ind(p, c, true).dirty = true;
            fresh = true;
        }
        return c;
    }
    std::uint32_t mapForWrite(Inode& in, std::uint32_t l, Pending& p, bool& fresh)
    {
        const std::uint32_t ppb = bs_ / 4, goal = group(in.ino);
        auto top = [&](int slot, bool isIndirect) {
            fresh = false;
            if (!in.block(slot)) {
                in.setBlock(slot, allocBit(false, goal));
                ++p.allocated;
                fresh = true;
                if (isIndirect) ind(p, in.block(slot), true).dirty = true;
            }
            return in.block(slot);
        };
        if (l < 12) return top(static_cast<int>(l), false);
        l -= 12;
        if (l < ppb) return childFor(p, top(12, true), l, false, goal, fresh);
        l -= ppb;
        if (l >= ppb * ppb) throw std::runtime_error("triple indirect blocks are not supported here");
        const std::uint32_t s = childFor(p, top(13, true), l / ppb, true, goal, fresh);
        return childFor(p, s, l % ppb, false, goal, fresh);
    }
    // Clears pointers from index `from` on in an indirect block of the given depth; collects
    // the blocks to free. Returns true when the whole indirect block goes away.
    bool cutIndirect(std::uint32_t b, std::uint32_t from, int depth, std::vector<std::uint32_t>& gone,
                     std::vector<std::pair<std::uint32_t, Bytes>>& rewrites)
    {
        const std::uint32_t ppb = bs_ / 4, span = depth == 1 ? 1 : ppb;
        Bytes v = blk(b);
        bool changed = false;
        for (std::uint32_t i = from / span; i < ppb; ++i) {
            const std::uint32_t c = get32(&v[i * 4]);
            if (!c) continue;
            const std::uint32_t childFrom = i * span >= from ? 0 : from - i * span;
            if (depth == 1 || cutIndirect(c, childFrom, 1, gone, rewrites)) {
                if (depth == 1) gone.push_back(c);
                put32(&v[i * 4], 0);
                changed = true;
            }
        }
        if (from == 0) {
            gone.push_back(b);
            return true;
        }
        if (changed) rewrites.emplace_back(b, v);
        return false;
    }
    std::vector<std::uint32_t> allBlocks(const Inode& in)
    {
        std::vector<std::uint32_t> all;
        std::vector<std::pair<std::uint32_t, Bytes>> unused;
        if (in.isFastLink()) return all;
        for (int i = 0; i < 12; ++i) {
            if (in.block(i)) all.push_back(in.block(i));
        }
        if (in.block(12)) cutIndirect(in.block(12), 0, 1, all, unused);
        if (in.block(13)) cutIndirect(in.block(13), 0, 2, all, unused);
        return all;
    }

    // Directories: entries are {inode u32, rec_len u16, name_len u8, file_type u8, name}.
    static std::uint32_t recLen(std::size_t nameLen) { return (8 + static_cast<std::uint32_t>(nameLen) + 3) & ~3u; }
    static void putEntry(std::uint8_t* e, std::uint32_t ino, std::uint32_t rl, const std::string& n, std::uint8_t ft)
    {
        put32(e, ino);
        put16(e + 4, rl);
        e[6] = static_cast<std::uint8_t>(n.size());
        e[7] = ft;
        std::copy(n.begin(), n.end(), e + 8);
    }
    std::uint32_t lookup(std::uint32_t dir, const std::string& name)
    {
        Inode d = readInode(dir);
        if (!d.isDir()) return 0;
        for (std::uint32_t l = 0; l * bs_ < d.size(); ++l) {
            Bytes v = blk(bmap(d, l));
            for (std::uint32_t off = 0; off < bs_; off += get16(&v[off + 4])) {
                const std::uint8_t* e = &v[off];
                if (get32(e) && e[6] == name.size() && std::equal(name.begin(), name.end(), e + 8)) return get32(e);
                if (get16(e + 4) == 0) break;
            }
        }
        return 0;
    }
    void addEntry(Inode& d, const std::string& name, std::uint32_t ino, std::uint8_t ft)
    {
        if (d.flags() & kIndexFl) throw std::runtime_error("hashed (indexed) directories are not supported here");
        if (name.empty() || name.size() > 255 || name.find('/') != std::string::npos)
            throw std::runtime_error("bad name");
        const std::uint32_t need = recLen(name.size());
        for (std::uint32_t l = 0; l * bs_ < d.size(); ++l) {
            const std::uint32_t b = bmap(d, l);
            Bytes v = blk(b);
            for (std::uint32_t off = 0; off < bs_; off += get16(&v[off + 4])) {
                std::uint8_t* e = &v[off];
                const std::uint32_t rl = get16(e + 4), used = get32(e) ? recLen(e[6]) : 0;
                if (rl - used >= need) {
                    if (used) put16(e + 4, used);                 // split the entry's slack
                    putEntry(&v[off + used], ino, rl - used, name, ft);
                    s_.meta(b, v.data());
                    return;
                }
            }
        }
        const std::uint32_t l = d.size() / bs_;
        if (l >= 12) throw std::runtime_error("directory full (only 12 blocks supported here)");
        const std::uint32_t b = allocBit(false, group(d.ino));
        Bytes v(bs_, 0);
        putEntry(&v[0], ino, bs_, name, ft);
        s_.meta(b, v.data());                                     // new block complete ...
        s_.order();
        d.setBlock(static_cast<int>(l), b);                       // ... before the inode points to it
        d.setSize(d.size() + bs_);
        d.setSectors(d.sectors() + bs_ / 512);
        writeInode(d);
    }
    void removeEntry(Inode& d, const std::string& name)
    {
        for (std::uint32_t l = 0; l * bs_ < d.size(); ++l) {
            const std::uint32_t b = bmap(d, l);
            Bytes v = blk(b);
            std::uint32_t prev = bs_;
            for (std::uint32_t off = 0; off < bs_; prev = off, off += get16(&v[off + 4])) {
                std::uint8_t* e = &v[off];
                if (!get32(e) || e[6] != name.size() || !std::equal(name.begin(), name.end(), e + 8)) continue;
                if (prev == bs_) put32(e, 0);                     // first in block: mark unused
                else put16(&v[prev + 4], get16(&v[prev + 4]) + get16(e + 4));   // merge into previous
                s_.meta(b, v.data());
                return;
            }
        }
        throw std::runtime_error("entry not found: " + name);
    }

    Store& s_;
    std::uint32_t bs_;
    bool careless_;
    Bytes sb_, gdt_;
    std::uint32_t sbBlock_ = 0, sbOff_ = 0, firstData_ = 0, bpg_ = 0, ipg_ = 0, isz_ = 0, groups_ = 0, gdtBlock_ = 0;
};

} // namespace os401
