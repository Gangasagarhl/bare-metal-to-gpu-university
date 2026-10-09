// fat32.cc - F3-32: the FAT32 driver. Offsets and rules after the Microsoft Extensible Firmware
// Initiative FAT32 File System Specification (BPB, FAT, directory entries, long names, FSInfo).
#include "fat32.h"
#include <algorithm>
#include <cstring>

namespace {
constexpr std::uint32_t EOC = 0x0FFFFFFF;          // end-of-chain mark written by this driver
constexpr std::uint8_t ATTR_LFN = 0x0F, ATTR_DIR = 0x10, ATTR_ARCHIVE = 0x20;

std::uint16_t le16(const std::uint8_t* p) { return static_cast<std::uint16_t>(p[0] | p[1] << 8); }
std::uint32_t le32(const std::uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<std::uint32_t>(p[3]) << 24; }
void put16(std::uint8_t* p, std::uint32_t v) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
void put32(std::uint8_t* p, std::uint32_t v) { put16(p, v); put16(p + 2, v >> 16); }

std::string lower(std::string s)
{
    for (char& c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}
} // namespace

BlockDev::BlockDev(const char* path) : f_(std::fopen(path, "r+b")) {}
BlockDev::~BlockDev() { if (f_) std::fclose(f_); }

void BlockDev::read(std::uint32_t lba, std::uint8_t* buf)
{
    std::fseek(f_, static_cast<long>(lba) * 512, SEEK_SET);
    if (std::fread(buf, 1, 512, f_) != 512) std::memset(buf, 0, 512);
    ++reads;
}

void BlockDev::write(std::uint32_t lba, const std::uint8_t* buf)
{
    std::fseek(f_, static_cast<long>(lba) * 512, SEEK_SET);
    std::fwrite(buf, 1, 512, f_);
    ++writes;
}

bool Fat32::mount()
{
    std::uint8_t s[512];
    dev_.read(0, s);
    if (s[510] != 0x55 || s[511] != 0xAA) return false;              // boot sector signature
    bytes_per_sec_ = le16(s + 11);
    sec_per_clus_ = s[13];
    reserved_ = le16(s + 14);
    num_fats_ = s[16];
    total_sec_ = le16(s + 19) ? le16(s + 19) : le32(s + 32);
    fat_size_ = le16(s + 22) ? le16(s + 22) : le32(s + 36);         // FAT32 uses the 32-bit field
    root_clus_ = le32(s + 44);
    fsinfo_sec_ = le16(s + 48);
    if (bytes_per_sec_ != 512 || le16(s + 17) != 0 || le16(s + 22) != 0) return false;  // FAT32 only
    data_lba_ = reserved_ + num_fats_ * fat_size_;
    cluster_count_ = (total_sec_ - data_lba_) / sec_per_clus_;        // the count decides FAT type
    if (cluster_count_ < 65525) return false;
    dev_.read(fsinfo_sec_, s);
    if (le32(s) != 0x41615252 || le32(s + 484) != 0x61417272) return false;
    free_count_ = le32(s + 488);
    next_free_ = le32(s + 492);
    if (free_count_ == 0xFFFFFFFF) {                                   // "unknown": count it
        free_count_ = 0;
        for (std::uint32_t c = 2; c < cluster_count_ + 2; ++c) if (fat_get(c) == 0) ++free_count_;
    }
    if (next_free_ < 2 || next_free_ >= cluster_count_ + 2) next_free_ = 2;
    return true;
}

void Fat32::print_info() const
{
    std::printf("bytes/sector %u, sectors/cluster %u, reserved sectors %u, FATs %u, FAT size %u sectors\n",
                bytes_per_sec_, sec_per_clus_, reserved_, num_fats_, fat_size_);
    std::printf("total sectors %u, first data sector %u, clusters %u, root cluster %u, FSInfo sector %u\n",
                total_sec_, data_lba_, cluster_count_, root_clus_, fsinfo_sec_);
    std::printf("free clusters %u, next-free hint %u\n", free_count_, next_free_);
}

std::uint32_t Fat32::fat_get(std::uint32_t c)
{
    std::uint8_t s[512];
    dev_.read(reserved_ + c * 4 / 512, s);                             // 4 bytes per entry
    return le32(s + c * 4 % 512) & 0x0FFFFFFF;                         // top 4 bits are reserved
}

void Fat32::fat_set(std::uint32_t c, std::uint32_t v)
{
    std::uint8_t s[512];
    for (std::uint32_t f = 0; f < num_fats_; ++f) {                    // keep every FAT copy equal
        std::uint32_t lba = reserved_ + f * fat_size_ + c * 4 / 512;
        dev_.read(lba, s);
        std::uint8_t* p = s + c * 4 % 512;
        put32(p, (le32(p) & 0xF0000000) | (v & 0x0FFFFFFF));          // preserve the reserved bits
        dev_.write(lba, s);
    }
}

std::uint32_t Fat32::alloc_cluster()
{
    for (std::uint32_t i = 0; i < cluster_count_; ++i) {
        std::uint32_t c = 2 + (next_free_ - 2 + i) % cluster_count_;
        if (fat_get(c) == 0) {
            fat_set(c, EOC);
            --free_count_;
            next_free_ = c + 1 < cluster_count_ + 2 ? c + 1 : 2;
            return c;
        }
    }
    return 0;                                                          // disk full
}

void Fat32::free_chain(std::uint32_t c)
{
    while (c >= 2 && c < 0x0FFFFFF8) {
        std::uint32_t next = fat_get(c);
        fat_set(c, 0);
        ++free_count_;
        c = next;
    }
}

std::vector<Fat32::Found> Fat32::read_dir(std::uint32_t dir_cluster)
{
    std::vector<Found> out;
    std::string lfn;                                                   // long name being assembled
    std::vector<Slot> lfn_slots;
    int lfn_sum = -1;
    std::uint8_t s[512];
    for (std::uint32_t c = dir_cluster; c >= 2 && c < 0x0FFFFFF8; c = fat_get(c)) {
        for (std::uint32_t sec = 0; sec < sec_per_clus_; ++sec) {
            dev_.read(cluster_lba(c) + sec, s);
            for (std::uint32_t i = 0; i < 16; ++i) {
                const std::uint8_t* e = s + i * 32;
                Slot slot{c, sec * 16 + i};
                if (e[0] == 0x00) return out;                          // end of directory
                if (e[0] == 0xE5) { lfn.clear(); lfn_slots.clear(); continue; }   // deleted
                if (e[11] == ATTR_LFN) {
                    if (e[0] & 0x40) { lfn.clear(); lfn_slots.clear(); lfn_sum = e[13]; }
                    std::string part;                                  // 13 UCS-2 characters
                    const int offs[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
                    for (int k : offs) {
                        std::uint16_t ch = le16(e + k);
                        if (ch == 0 || ch == 0xFFFF) break;
                        part += ch < 0x80 ? static_cast<char>(ch) : '?';
                    }
                    lfn = part + lfn;                                  // stored last part first
                    lfn_slots.push_back(slot);
                    continue;
                }
                if (e[11] & 0x08) { lfn.clear(); lfn_slots.clear(); continue; }   // volume label
                ShortName sn;
                std::memcpy(sn.data(), e, 11);
                Found f;
                f.e.short_name = short_to_string(sn);
                f.e.attr = e[11];
                f.e.first_cluster = static_cast<std::uint32_t>(le16(e + 20)) << 16 | le16(e + 26);
                f.e.size = le32(e + 28);
                if (!lfn.empty() && lfn_sum == lfn_checksum(sn)) {     // the long name belongs here
                    f.e.long_name = lfn;
                    f.slots = lfn_slots;
                }
                f.slots.push_back(slot);
                out.push_back(f);
                lfn.clear();
                lfn_slots.clear();
            }
        }
    }
    return out;
}

// Follow a path such as "/Docs/notes.txt". Returns the entry; parent gets the cluster of the
// directory that holds the last component (also when that component does not exist yet).
std::optional<Entry> Fat32::walk(const std::string& path, std::uint32_t* parent, std::string* last)
{
    std::uint32_t dir = root_clus_;
    Entry cur{"", "/", ATTR_DIR, root_clus_, 0};
    std::size_t pos = 0;
    while (pos < path.size()) {
        while (pos < path.size() && path[pos] == '/') ++pos;
        if (pos == path.size()) break;
        std::size_t end = path.find('/', pos);
        std::string comp = path.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        pos = end == std::string::npos ? path.size() : end;
        if (!cur.is_dir()) return std::nullopt;
        dir = cur.first_cluster ? cur.first_cluster : root_clus_;     // ".." to the root stores 0
        if (parent) *parent = dir;
        if (last) *last = comp;
        bool hit = false;
        for (const Found& f : read_dir(dir)) {                          // names compare without case
            if (lower(f.e.long_name) == lower(comp) || lower(f.e.short_name) == lower(comp)) {
                cur = f.e;
                hit = true;
                break;
            }
        }
        if (!hit) return std::nullopt;
    }
    return cur;
}

std::optional<std::vector<Entry>> Fat32::list(const std::string& path)
{
    auto e = walk(path, nullptr, nullptr);
    if (!e || !e->is_dir()) return std::nullopt;
    std::vector<Entry> out;
    for (const Found& f : read_dir(e->first_cluster ? e->first_cluster : root_clus_)) out.push_back(f.e);
    return out;
}

std::optional<std::vector<std::uint8_t>> Fat32::read_file(const std::string& path)
{
    auto e = walk(path, nullptr, nullptr);
    if (!e || e->is_dir()) return std::nullopt;
    std::vector<std::uint8_t> data;
    std::uint8_t s[512];
    for (std::uint32_t c = e->first_cluster; c >= 2 && c < 0x0FFFFFF8 && data.size() < e->size; c = fat_get(c)) {
        for (std::uint32_t sec = 0; sec < sec_per_clus_ && data.size() < e->size; ++sec) {
            dev_.read(cluster_lba(c) + sec, s);
            std::size_t n = std::min<std::size_t>(512, e->size - data.size());
            data.insert(data.end(), s, s + n);
        }
    }
    return data;
}

void Fat32::write_slot(const Slot& sl, const std::uint8_t* entry32)
{
    std::uint8_t s[512];
    std::uint32_t lba = cluster_lba(sl.cluster) + sl.index / 16;
    dev_.read(lba, s);
    std::memcpy(s + sl.index % 16 * 32, entry32, 32);
    dev_.write(lba, s);
}

// Find n consecutive free entries (deleted or never used) in a directory; grow it if needed.
std::vector<Fat32::Slot> Fat32::free_slots(std::uint32_t dir_cluster, std::size_t n)
{
    std::vector<Slot> run;
    std::uint32_t c = dir_cluster, last = dir_cluster;
    std::uint8_t s[512];
    for (; c >= 2 && c < 0x0FFFFFF8; last = c, c = fat_get(c)) {
        for (std::uint32_t sec = 0; sec < sec_per_clus_; ++sec) {
            dev_.read(cluster_lba(c) + sec, s);
            for (std::uint32_t i = 0; i < 16; ++i) {
                if (s[i * 32] == 0x00 || s[i * 32] == 0xE5) {
                    run.push_back(Slot{c, sec * 16 + i});
                    if (run.size() == n) return run;
                } else {
                    run.clear();
                }
            }
        }
    }
    while (run.size() < n) {                                          // extend the directory
        std::uint32_t nc = alloc_cluster();
        if (nc == 0) return {};
        std::memset(s, 0, sizeof s);                                  // a new cluster must be zeroed
        for (std::uint32_t sec = 0; sec < sec_per_clus_; ++sec) dev_.write(cluster_lba(nc) + sec, s);
        fat_set(last, nc);
        last = nc;
        for (std::uint32_t i = 0; i < sec_per_clus_ * 16 && run.size() < n; ++i) run.push_back(Slot{nc, i});
    }
    return run;
}

// Write data into newly allocated clusters; return the first cluster (0 for an empty file).
std::uint32_t Fat32::write_chain(const std::vector<std::uint8_t>& data)
{
    std::uint32_t first = 0, prev = 0;
    std::size_t clus_bytes = sec_per_clus_ * 512;
    for (std::size_t off = 0; off < data.size(); off += clus_bytes) {
        std::uint32_t c = alloc_cluster();
        if (c == 0) { free_chain(first); return 0xFFFFFFFF; }
        for (std::uint32_t sec = 0; sec < sec_per_clus_; ++sec) {
            std::uint8_t s[512] = {};
            std::size_t at = off + sec * 512;
            if (at < data.size()) std::memcpy(s, data.data() + at, std::min<std::size_t>(512, data.size() - at));
            dev_.write(cluster_lba(c) + sec, s);
        }
        if (prev) fat_set(prev, c); else first = c;
        prev = c;
    }
    return first;
}

int Fat32::write_file(const std::string& path, const std::vector<std::uint8_t>& data)
{
    std::uint32_t parent = root_clus_;
    std::string name;
    auto existing = walk(path, &parent, &name);
    if (existing && existing->is_dir()) return -1;
    // Order chosen so that a crash leaves at worst lost clusters, never a name that points
    // to unwritten or freed data: 1) data and its chain, 2) the directory entry, 3) free the old chain.
    std::uint32_t first = write_chain(data);
    if (first == 0xFFFFFFFF) return -2;
    std::uint8_t e[32] = {};
    const std::uint32_t date = (2026 - 1980) << 9 | 10 << 5 | 9, time = 12 << 11;  // fixed time stamp
    if (existing) {                                                   // replace: update the 8.3 entry
        for (const Found& f : read_dir(parent)) {
            if (f.e.short_name != existing->short_name) continue;
            const Slot& sl = f.slots.back();
            std::uint8_t s[512];
            dev_.read(cluster_lba(sl.cluster) + sl.index / 16, s);
            std::memcpy(e, s + sl.index % 16 * 32, 32);
            put16(e + 20, first >> 16);
            put16(e + 26, first & 0xFFFF);
            put32(e + 28, static_cast<std::uint32_t>(data.size()));
            put16(e + 22, time);
            put16(e + 24, date);
            write_slot(sl, e);
            free_chain(existing->first_cluster);
            write_fsinfo();
            return 0;
        }
        return -3;
    }
    bool lossy = false, recased = false;
    ShortName sn = short_basis(name, lossy, recased);
    auto entries = read_dir(parent);
    auto taken = [&](const ShortName& cand) {
        for (const Found& f : entries) if (f.e.short_name == short_to_string(cand)) return true;
        return false;
    };
    if (lossy || taken(sn)) {                                         // numeric tail ~1, ~2, ...
        int n = 1;
        while (taken(with_tail(sn, n))) ++n;
        sn = with_tail(sn, n);
    }
    bool need_lfn = lossy || recased;
    std::size_t nlfn = need_lfn ? (name.size() + 12) / 13 : 0;
    auto slots = free_slots(parent, nlfn + 1);
    if (slots.empty()) { free_chain(first); return -2; }
    std::uint8_t sum = lfn_checksum(sn);
    for (std::size_t k = 0; k < nlfn; ++k) {                          // physical order: last part first
        std::size_t part = nlfn - 1 - k;
        std::uint8_t l[32] = {};
        l[0] = static_cast<std::uint8_t>((part + 1) | (k == 0 ? 0x40 : 0));
        l[11] = ATTR_LFN;
        l[13] = sum;
        const int offs[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
        for (int j = 0; j < 13; ++j) {
            std::size_t at = part * 13 + static_cast<std::size_t>(j);
            std::uint16_t ch = at < name.size() ? static_cast<std::uint8_t>(name[at]) : at == name.size() ? 0 : 0xFFFF;
            put16(l + offs[j], ch);                                   // NUL after the name, then 0xFFFF padding
        }
        write_slot(slots[k], l);
    }
    std::memcpy(e, sn.data(), 11);
    e[11] = ATTR_ARCHIVE;
    put16(e + 14, time); put16(e + 16, date); put16(e + 18, date);
    put16(e + 20, first >> 16);
    put16(e + 22, time); put16(e + 24, date);
    put16(e + 26, first & 0xFFFF);
    put32(e + 28, static_cast<std::uint32_t>(data.size()));
    write_slot(slots[nlfn], e);                                        // the file becomes visible here
    write_fsinfo();
    return 0;
}

int Fat32::remove(const std::string& path)
{
    std::uint32_t parent = root_clus_;
    std::string name;
    auto e = walk(path, &parent, &name);
    if (!e || e->is_dir()) return -1;
    for (const Found& f : read_dir(parent)) {
        if (f.e.short_name != e->short_name) continue;
        for (const Slot& sl : f.slots) {                               // 1) hide the name ...
            std::uint8_t s[512];
            std::uint32_t lba = cluster_lba(sl.cluster) + sl.index / 16;
            dev_.read(lba, s);
            s[sl.index % 16 * 32] = 0xE5;
            dev_.write(lba, s);
        }
        free_chain(e->first_cluster);                                  // 2) ... then free the data
        write_fsinfo();
        return 0;
    }
    return -1;
}

void Fat32::write_fsinfo()
{
    std::uint8_t s[512];
    dev_.read(fsinfo_sec_, s);
    put32(s + 488, free_count_);
    put32(s + 492, next_free_);
    dev_.write(fsinfo_sec_, s);
}
