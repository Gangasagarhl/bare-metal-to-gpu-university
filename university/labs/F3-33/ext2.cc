// ext2.cc - F3-33: the ext2 driver. On-disk layout after "The Second Extended File System:
// Internal Layout" (Dave Poirier) and the Linux kernel's ext4 on-disk format documentation.
#include "ext2.h"
#include <algorithm>
#include <cstring>
#include <ctime>

namespace {
std::uint16_t le16(const std::uint8_t* p) { return static_cast<std::uint16_t>(p[0] | p[1] << 8); }
std::uint32_t le32(const std::uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<std::uint32_t>(p[3]) << 24; }
void put16(std::uint8_t* p, std::uint32_t v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
void put32(std::uint8_t* p, std::uint32_t v) { put16(p, v); put16(p + 2, v >> 16); }
std::uint32_t now() { return static_cast<std::uint32_t>(std::time(nullptr)); }
constexpr std::uint32_t ROOT_INO = 2;
constexpr std::uint8_t FT_FILE = 1, FT_DIR = 2;   // directory entry file types (filetype feature)
} // namespace

Disk::Disk(const char* path, std::uint32_t bs) : f_(std::fopen(path, "r+b")), bs_(bs) {}
Disk::~Disk() { if (f_) std::fclose(f_); }

void Disk::read_bytes(std::uint64_t off, std::uint8_t* buf, std::uint32_t n)
{
    std::fseek(f_, static_cast<long>(off), SEEK_SET);
    if (std::fread(buf, 1, n, f_) != n) std::memset(buf, 0, n);
}

void Disk::read(std::uint32_t b, std::uint8_t* buf) { read_bytes(std::uint64_t{b} * bs_, buf, bs_); }

void Disk::write(std::uint32_t b, const std::uint8_t* buf)
{
    bool reaches = cut_after < 0 || static_cast<long>(writes) < cut_after;   // after the cut: lost
    ++writes;
    log.push_back(Write{b, reaches});
    if (!reaches) { ++lost; return; }
    std::fseek(f_, static_cast<long>(std::uint64_t{b} * bs_), SEEK_SET);
    std::fwrite(buf, 1, bs_, f_);
    std::fflush(f_);
}

bool Ext2::mount()
{
    d_.read_bytes(1024, sb_, 1024);                         // the superblock is always at byte 1024
    if (le16(sb_ + 56) != 0xEF53) return false;             // s_magic
    inodes_count_ = le32(sb_ + 0);
    blocks_count_ = le32(sb_ + 4);
    free_blocks_ = le32(sb_ + 12);
    free_inodes_ = le32(sb_ + 16);
    first_data_ = le32(sb_ + 20);
    bs_ = 1024u << le32(sb_ + 24);
    blocks_per_group_ = le32(sb_ + 32);
    inodes_per_group_ = le32(sb_ + 40);
    if (le32(sb_ + 76) >= 1) {                              // revision 1: dynamic inode size
        first_ino_ = le32(sb_ + 84);
        inode_size_ = le16(sb_ + 88);
    }
    std::uint32_t incompat = le32(sb_ + 96), ro_compat = le32(sb_ + 100);
    if ((incompat & ~0x0002u) != 0 || (ro_compat & ~0x0003u) != 0) return false;  // only filetype; sparse_super, large_file
    d_.set_block_size(bs_);
    ngroups_ = (blocks_count_ - first_data_ + blocks_per_group_ - 1) / blocks_per_group_;
    std::vector<std::uint8_t> b(bs_);
    for (std::uint32_t g = 0; g < ngroups_; ++g) {          // descriptors follow the superblock's block
        if (g * 32 % bs_ == 0) d_.read(first_data_ + 1 + g * 32 / bs_, b.data());
        const std::uint8_t* p = b.data() + g * 32 % bs_;
        groups_.push_back(Group{le32(p), le32(p + 4), le32(p + 8), le16(p + 12), le16(p + 14), le16(p + 16)});
    }
    return true;
}

void Ext2::print_info() const
{
    std::printf("block size %u, blocks %u, inodes %u, first data block %u, groups %u\n",
                bs_, blocks_count_, inodes_count_, first_data_, ngroups_);
    std::printf("blocks per group %u, inodes per group %u, inode size %u, first non-reserved inode %u\n",
                blocks_per_group_, inodes_per_group_, inode_size_, first_ino_);
    std::printf("free blocks %u, free inodes %u; features compat 0x%x incompat 0x%x ro_compat 0x%x\n",
                free_blocks_, free_inodes_, le32(sb_ + 92), le32(sb_ + 96), le32(sb_ + 100));
    for (std::uint32_t g = 0; g < ngroups_; ++g)
        std::printf("group %u: block bitmap %u, inode bitmap %u, inode table %u, free blocks %u, free inodes %u, dirs %u\n",
                    g, groups_[g].block_bitmap, groups_[g].inode_bitmap, groups_[g].inode_table,
                    groups_[g].free_blocks, groups_[g].free_inodes, groups_[g].used_dirs);
}

std::string Ext2::role(std::uint32_t b) const
{
    if (b == 1024 / bs_) return "superblock";
    if (b == first_data_ + 1) return "group descriptors";
    for (std::uint32_t g = 0; g < ngroups_; ++g) {
        const Group& gr = groups_[g];
        if (b == gr.block_bitmap) return "block bitmap, group " + std::to_string(g);
        if (b == gr.inode_bitmap) return "inode bitmap, group " + std::to_string(g);
        if (b >= gr.inode_table && b < gr.inode_table + inodes_per_group_ * inode_size_ / bs_)
            return "inode table, group " + std::to_string(g);
    }
    return "data, directory or indirect block";
}

Inode Ext2::read_inode(std::uint32_t ino)
{
    std::uint32_t g = (ino - 1) / inodes_per_group_, idx = (ino - 1) % inodes_per_group_;
    std::uint8_t r[128];
    d_.read_bytes(std::uint64_t{groups_[g].inode_table} * bs_ + std::uint64_t{idx} * inode_size_, r, 128);
    Inode in;
    in.mode = le16(r);
    in.size = le32(r + 4);
    in.dtime = le32(r + 20);
    in.links = le16(r + 26);
    in.blocks512 = le32(r + 28);
    for (int i = 0; i < 15; ++i) in.block[i] = le32(r + 40 + 4 * i);
    return in;
}

void Ext2::write_inode(std::uint32_t ino, const Inode& in)
{
    std::uint32_t g = (ino - 1) / inodes_per_group_, idx = (ino - 1) % inodes_per_group_;
    std::uint64_t off = std::uint64_t{groups_[g].inode_table} * bs_ + std::uint64_t{idx} * inode_size_;
    std::uint32_t blk = static_cast<std::uint32_t>(off / bs_);
    std::vector<std::uint8_t> b(bs_);
    d_.read(blk, b.data());                                // read-modify-write of the table block
    std::uint8_t* r = b.data() + off % bs_;
    std::memset(r, 0, 128);
    std::uint32_t t = now();
    put16(r, in.mode);
    put32(r + 4, in.size);
    put32(r + 8, t); put32(r + 12, t); put32(r + 16, t);   // atime, ctime, mtime
    put32(r + 20, in.dtime);
    put16(r + 26, in.links);
    put32(r + 28, in.blocks512);                           // counted in 512-byte units
    for (int i = 0; i < 15; ++i) put32(r + 40 + 4 * i, in.block[i]);
    d_.write(blk, b.data());
}

std::uint32_t Ext2::bmap(const Inode& in, std::uint32_t i)
{
    const std::uint32_t p = bs_ / 4;                        // block pointers per indirect block
    std::vector<std::uint8_t> b(bs_);
    if (i < 12) return in.block[i];
    i -= 12;
    if (i < p) {
        if (!in.block[12]) return 0;
        d_.read(in.block[12], b.data());
        return le32(b.data() + 4 * i);
    }
    i -= p;
    if (i < p * p) {
        if (!in.block[13]) return 0;
        d_.read(in.block[13], b.data());
        std::uint32_t l2 = le32(b.data() + 4 * (i / p));
        if (!l2) return 0;
        d_.read(l2, b.data());
        return le32(b.data() + 4 * (i % p));
    }
    return 0;                                               // triple indirect: not supported here
}

std::vector<std::uint32_t> Ext2::all_blocks(const Inode& in)
{
    std::vector<std::uint32_t> out;
    std::vector<std::uint8_t> b(bs_), c(bs_);
    const std::uint32_t p = bs_ / 4;
    for (int i = 0; i < 12; ++i) if (in.block[i]) out.push_back(in.block[i]);
    if (in.block[12]) {
        out.push_back(in.block[12]);
        d_.read(in.block[12], b.data());
        for (std::uint32_t k = 0; k < p; ++k) if (le32(b.data() + 4 * k)) out.push_back(le32(b.data() + 4 * k));
    }
    if (in.block[13]) {
        out.push_back(in.block[13]);
        d_.read(in.block[13], b.data());
        for (std::uint32_t k = 0; k < p; ++k) {
            std::uint32_t l2 = le32(b.data() + 4 * k);
            if (!l2) continue;
            out.push_back(l2);
            d_.read(l2, c.data());
            for (std::uint32_t j = 0; j < p; ++j) if (le32(c.data() + 4 * j)) out.push_back(le32(c.data() + 4 * j));
        }
    }
    return out;
}

std::vector<DirEntry> Ext2::read_dir(std::uint32_t ino)
{
    std::vector<DirEntry> out;
    Inode dir = read_inode(ino);
    std::vector<std::uint8_t> b(bs_);
    for (std::uint32_t i = 0; i < dir.size / bs_; ++i) {
        d_.read(bmap(dir, i), b.data());
        for (std::uint32_t off = 0; off < bs_;) {
            const std::uint8_t* e = b.data() + off;
            std::uint16_t rec_len = le16(e + 4);
            if (rec_len < 8) break;                         // damaged block: stop, do not loop
            if (le32(e)) out.push_back(DirEntry{le32(e), e[7], std::string(reinterpret_cast<const char*>(e + 8), e[6])});
            off += rec_len;
        }
    }
    return out;
}

std::optional<std::uint32_t> Ext2::lookup(const std::string& path, std::uint32_t* parent, std::string* last)
{
    std::uint32_t cur = ROOT_INO;
    std::size_t pos = 0;
    while (pos < path.size()) {
        while (pos < path.size() && path[pos] == '/') ++pos;
        if (pos == path.size()) break;
        std::size_t end = path.find('/', pos);
        std::string comp = path.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        pos = end == std::string::npos ? path.size() : end;
        if (!read_inode(cur).is_dir()) return std::nullopt;
        if (parent) *parent = cur;
        if (last) *last = comp;
        std::uint32_t next = 0;
        for (const DirEntry& e : read_dir(cur)) if (e.name == comp) { next = e.inode; break; }  // case matters
        if (!next) return std::nullopt;
        cur = next;
    }
    return cur;
}

std::optional<std::vector<DirEntry>> Ext2::list(const std::string& path)
{
    auto ino = lookup(path, nullptr, nullptr);
    if (!ino || !read_inode(*ino).is_dir()) return std::nullopt;
    return read_dir(*ino);
}

std::optional<std::vector<std::uint8_t>> Ext2::read_file(const std::string& path)
{
    auto ino = lookup(path, nullptr, nullptr);
    if (!ino) return std::nullopt;
    Inode in = read_inode(*ino);
    if (in.is_dir()) return std::nullopt;
    std::vector<std::uint8_t> out, b(bs_);
    for (std::uint32_t i = 0; out.size() < in.size; ++i) {
        std::uint32_t blk = bmap(in, i);
        if (blk) d_.read(blk, b.data()); else std::fill(b.begin(), b.end(), 0);   // a hole reads as zeros
        std::size_t n = std::min<std::size_t>(bs_, in.size - out.size());
        out.insert(out.end(), b.begin(), b.begin() + static_cast<long>(n));
    }
    return out;
}

void Ext2::write_group(std::uint32_t g)
{
    std::vector<std::uint8_t> b(bs_);
    std::uint32_t blk = first_data_ + 1 + g * 32 / bs_;
    d_.read(blk, b.data());
    std::uint8_t* p = b.data() + g * 32 % bs_;
    put16(p + 12, groups_[g].free_blocks);
    put16(p + 14, groups_[g].free_inodes);
    put16(p + 16, groups_[g].used_dirs);
    d_.write(blk, b.data());
}

void Ext2::write_super()
{
    put32(sb_ + 12, free_blocks_);
    put32(sb_ + 16, free_inodes_);
    std::vector<std::uint8_t> b(bs_);
    std::uint32_t blk = 1024 / bs_;
    d_.read(blk, b.data());
    std::memcpy(b.data() + 1024 % bs_, sb_, 1024);
    d_.write(blk, b.data());
}

std::uint32_t Ext2::alloc_inode(bool dir)
{
    std::vector<std::uint8_t> b(bs_);
    for (std::uint32_t g = 0; g < ngroups_; ++g) {
        if (groups_[g].free_inodes == 0) continue;
        d_.read(groups_[g].inode_bitmap, b.data());
        for (std::uint32_t i = 0; i < inodes_per_group_; ++i) {
            std::uint32_t ino = g * inodes_per_group_ + i + 1;
            if (ino < first_ino_ || (b[i / 8] >> (i % 8) & 1)) continue;
            b[i / 8] = static_cast<std::uint8_t>(b[i / 8] | 1 << (i % 8));
            d_.write(groups_[g].inode_bitmap, b.data());
            --groups_[g].free_inodes;
            if (dir) ++groups_[g].used_dirs;
            --free_inodes_;
            write_group(g);
            write_super();
            return ino;
        }
    }
    return 0;
}

void Ext2::free_inode(std::uint32_t ino, bool dir)
{
    std::uint32_t g = (ino - 1) / inodes_per_group_, i = (ino - 1) % inodes_per_group_;
    std::vector<std::uint8_t> b(bs_);
    d_.read(groups_[g].inode_bitmap, b.data());
    b[i / 8] = static_cast<std::uint8_t>(b[i / 8] & ~(1 << (i % 8)));
    d_.write(groups_[g].inode_bitmap, b.data());
    ++groups_[g].free_inodes;
    if (dir) --groups_[g].used_dirs;
    ++free_inodes_;
    write_group(g);
    write_super();
}

std::vector<std::uint32_t> Ext2::pick_blocks(std::uint32_t n)
{
    std::vector<std::uint32_t> out;
    std::vector<std::uint8_t> b(bs_);
    for (std::uint32_t g = 0; g < ngroups_ && out.size() < n; ++g) {
        if (groups_[g].free_blocks == 0) continue;
        d_.read(groups_[g].block_bitmap, b.data());
        for (std::uint32_t i = 0; i < blocks_per_group_ && out.size() < n; ++i) {
            std::uint32_t blk = first_data_ + g * blocks_per_group_ + i;
            if (blk >= blocks_count_) break;
            if (!(b[i / 8] >> (i % 8) & 1)) out.push_back(blk);
        }
    }
    return out;
}

void Ext2::mark_blocks(const std::vector<std::uint32_t>& blocks, bool used)
{
    std::vector<std::uint8_t> b(bs_);
    for (std::uint32_t g = 0; g < ngroups_; ++g) {          // one bitmap write per group touched
        bool touched = false;
        for (std::uint32_t blk : blocks) {
            std::uint32_t rel = blk - first_data_;
            if (rel / blocks_per_group_ != g) continue;
            if (!touched) { d_.read(groups_[g].block_bitmap, b.data()); touched = true; }
            std::uint32_t i = rel % blocks_per_group_;
            if (used) b[i / 8] = static_cast<std::uint8_t>(b[i / 8] | 1 << (i % 8));
            else b[i / 8] = static_cast<std::uint8_t>(b[i / 8] & ~(1 << (i % 8)));
            if (used) { --groups_[g].free_blocks; --free_blocks_; } else { ++groups_[g].free_blocks; ++free_blocks_; }
        }
        if (touched) { d_.write(groups_[g].block_bitmap, b.data()); write_group(g); }
    }
    if (!blocks.empty()) write_super();
}

std::uint32_t Ext2::blocks_needed(std::uint32_t n, std::uint32_t p)
{
    if (n <= 12) return n;
    if (n <= 12 + p) return n + 1;                          // + the single indirect block
    std::uint32_t rest = n - 12 - p;
    return n + 1 + 1 + (rest + p - 1) / p;                  // + single, double and its children
}

// Assign blocks in file order (each indirect block just before the data it maps), write the data
// blocks and the indirect blocks, fill in.block[] and in.blocks512. Returns blocks used.
std::uint32_t Ext2::layout(Inode& in, const std::vector<std::uint32_t>& blocks, std::uint32_t n,
                           const std::vector<std::uint8_t>& data)
{
    const std::uint32_t p = bs_ / 4;
    std::size_t next = 0;
    std::vector<std::uint8_t> single(bs_, 0), dbl(bs_, 0), l2(bs_, 0), b(bs_);
    std::uint32_t l2_blk = 0;
    auto flush_l2 = [&] { if (l2_blk) { d_.write(l2_blk, l2.data()); std::fill(l2.begin(), l2.end(), 0); } };
    for (std::uint32_t i = 0; i < n; ++i) {
        if (i == 12) in.block[12] = blocks[next++];
        if (i == 12 + p) in.block[13] = blocks[next++];
        if (i >= 12 + p && (i - 12 - p) % p == 0) {
            flush_l2();
            l2_blk = blocks[next++];
            put32(dbl.data() + 4 * ((i - 12 - p) / p), l2_blk);
        }
        std::uint32_t blk = blocks[next++];
        std::fill(b.begin(), b.end(), 0);
        std::size_t at = std::size_t{i} * bs_;
        std::memcpy(b.data(), data.data() + at, std::min<std::size_t>(bs_, data.size() - at));
        d_.write(blk, b.data());
        if (i < 12) in.block[i] = blk;
        else if (i < 12 + p) put32(single.data() + 4 * (i - 12), blk);
        else put32(l2.data() + 4 * ((i - 12 - p) % p), blk);
    }
    flush_l2();
    if (in.block[12]) d_.write(in.block[12], single.data());
    if (in.block[13]) d_.write(in.block[13], dbl.data());
    in.blocks512 = static_cast<std::uint32_t>(next) * (bs_ / 512);
    return static_cast<std::uint32_t>(next);
}

int Ext2::add_entry(std::uint32_t dir, const std::string& name, std::uint32_t ino, std::uint8_t type)
{
    const std::uint32_t need = (8 + static_cast<std::uint32_t>(name.size()) + 3) & ~3u;
    Inode d = read_inode(dir);
    std::vector<std::uint8_t> b(bs_);
    auto fill = [&](std::uint8_t* e, std::uint32_t rec_len) {
        put32(e, ino);
        put16(e + 4, rec_len);
        e[6] = static_cast<std::uint8_t>(name.size());
        e[7] = type;
        std::memcpy(e + 8, name.data(), name.size());
    };
    for (std::uint32_t i = 0; i < d.size / bs_; ++i) {
        std::uint32_t blk = bmap(d, i);
        d_.read(blk, b.data());
        for (std::uint32_t off = 0; off < bs_;) {
            std::uint8_t* e = b.data() + off;
            std::uint32_t rec_len = le16(e + 4);
            if (rec_len < 8) return -5;
            std::uint32_t used = le32(e) ? (8 + e[6] + 3) & ~3u : 0;
            if (rec_len - used >= need) {                   // room inside this entry's record
                if (used) { put16(e + 4, used); e = b.data() + off + used; rec_len -= used; }
                fill(e, rec_len);
                d_.write(blk, b.data());
                return 0;
            }
            off += rec_len;
        }
    }
    std::uint32_t idx = d.size / bs_;                       // no room: give the directory a new block
    if (idx >= 12) return -4;
    std::vector<std::uint32_t> nb = pick_blocks(1);
    if (nb.empty()) return -2;
    mark_blocks(nb, true);
    std::fill(b.begin(), b.end(), 0);
    fill(b.data(), bs_);
    d_.write(nb[0], b.data());
    d.block[idx] = nb[0];
    d.size += bs_;
    d.blocks512 += bs_ / 512;
    write_inode(dir, d);
    return 0;
}

int Ext2::remove_entry(std::uint32_t dir, const std::string& name)
{
    Inode d = read_inode(dir);
    std::vector<std::uint8_t> b(bs_);
    for (std::uint32_t i = 0; i < d.size / bs_; ++i) {
        std::uint32_t blk = bmap(d, i);
        d_.read(blk, b.data());
        std::uint8_t* prev = nullptr;
        for (std::uint32_t off = 0; off < bs_;) {
            std::uint8_t* e = b.data() + off;
            std::uint16_t rec_len = le16(e + 4);
            if (rec_len < 8) return -5;
            if (le32(e) && e[6] == name.size() && std::memcmp(e + 8, name.data(), name.size()) == 0) {
                if (prev) put16(prev + 4, le16(prev + 4) + rec_len);   // merge into the previous record
                else put32(e, 0);                                       // first in block: mark unused
                d_.write(blk, b.data());
                return 0;
            }
            prev = e;
            off += rec_len;
        }
    }
    return -1;
}

int Ext2::write_file(const std::string& path, const std::vector<std::uint8_t>& data)
{
    std::uint32_t parent = ROOT_INO;
    std::string name;
    auto existing = lookup(path, &parent, &name);
    if (name.empty() || name.size() > 255) return -1;
    Inode old;
    if (existing) { old = read_inode(*existing); if (old.is_dir()) return -1; }
    const std::uint32_t n = static_cast<std::uint32_t>((data.size() + bs_ - 1) / bs_);
    const std::uint32_t p = bs_ / 4;
    if (n > 12 + p + p * p) return -3;
    const std::uint32_t total = blocks_needed(n, p);
    std::vector<std::uint32_t> blocks = pick_blocks(total);
    if (blocks.size() != total) return -2;
    // Order: a power cut at any point leaves at worst blocks or an inode marked used that nothing
    // refers to (a leak e2fsck reports and repairs), never a name pointing to unmarked blocks.
    mark_blocks(blocks, true);                                       // 1) block bitmap and counts
    Inode in;
    in.mode = 0x81A4;                                                // regular file, rw-r--r--
    in.size = static_cast<std::uint32_t>(data.size());
    in.links = 1;
    layout(in, blocks, n, data);                                     // 2) data and indirect blocks
    if (existing) {
        in.links = old.links;
        write_inode(*existing, in);                                  // 3) the inode now uses the new blocks
        mark_blocks(all_blocks(old), false);                         // 4) only then free the old ones
        return 0;
    }
    std::uint32_t ino = alloc_inode(false);                          //    inode bitmap and counts
    if (!ino) { mark_blocks(blocks, false); return -2; }
    write_inode(ino, in);                                            // 3) the inode
    return add_entry(parent, name, ino, FT_FILE);                    // 4) the name
}

int Ext2::mkdir(const std::string& path)
{
    std::uint32_t parent = ROOT_INO;
    std::string name;
    if (lookup(path, &parent, &name) || name.empty()) return -1;
    std::vector<std::uint32_t> blk = pick_blocks(1);
    if (blk.empty()) return -2;
    mark_blocks(blk, true);
    std::uint32_t ino = alloc_inode(true);
    if (!ino) { mark_blocks(blk, false); return -2; }
    std::vector<std::uint8_t> b(bs_, 0);                             // "." and ".." entries
    put32(b.data(), ino); put16(b.data() + 4, 12); b[6] = 1; b[7] = FT_DIR; b[8] = '.';
    put32(b.data() + 12, parent); put16(b.data() + 16, bs_ - 12); b[18] = 2; b[19] = FT_DIR; b[20] = '.'; b[21] = '.';
    d_.write(blk[0], b.data());
    Inode in;
    in.mode = 0x41ED;                                                // directory, rwxr-xr-x
    in.size = bs_;
    in.links = 2;                                                    // its name and its own "."
    in.blocks512 = bs_ / 512;
    in.block[0] = blk[0];
    write_inode(ino, in);
    int rc = add_entry(parent, name, ino, FT_DIR);
    Inode pin = read_inode(parent);
    ++pin.links;                                                     // the child's ".." points here
    write_inode(parent, pin);
    return rc;
}

int Ext2::unlink(const std::string& path)
{
    std::uint32_t parent = ROOT_INO;
    std::string name;
    auto ino = lookup(path, &parent, &name);
    if (!ino) return -1;
    Inode in = read_inode(*ino);
    if (in.is_dir()) return -1;
    if (remove_entry(parent, name) != 0) return -1;                  // 1) the name goes first
    if (--in.links > 0) { write_inode(*ino, in); return 0; }
    std::vector<std::uint32_t> blocks = all_blocks(in);
    in.dtime = now();                                                // 2) the inode is marked deleted
    write_inode(*ino, in);
    mark_blocks(blocks, false);                                      // 3) blocks and inode are freed
    free_inode(*ino, false);
    return 0;
}
