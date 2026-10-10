// fat32.h - F3-32: a small FAT32 driver (read, create, replace, delete; long names).
// Host C++ over a disk image; in the kernel the BlockDev is the B14 block cache.
#pragma once
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>
#include "fat_names.h"

class BlockDev
{
public:
    explicit BlockDev(const char* path);
    ~BlockDev();
    BlockDev(const BlockDev&) = delete;
    BlockDev& operator=(const BlockDev&) = delete;
    bool ok() const { return f_ != nullptr; }
    void read(std::uint32_t lba, std::uint8_t* buf);            // one 512-byte sector
    void write(std::uint32_t lba, const std::uint8_t* buf);
    std::size_t reads = 0, writes = 0;
private:
    std::FILE* f_;
};

struct Entry                             // one file or directory as the driver reports it
{
    std::string long_name;               // empty when the file has only a short name
    std::string short_name;              // "NAME.EXT"
    std::uint8_t attr;
    std::uint32_t first_cluster;
    std::uint32_t size;
    bool is_dir() const { return attr & 0x10; }
    const std::string& name() const { return long_name.empty() ? short_name : long_name; }
};

class Fat32
{
public:
    explicit Fat32(BlockDev& dev) : dev_(dev) {}
    bool mount();                        // read and check the boot sector (BPB) and FSInfo
    void print_info() const;
    std::optional<std::vector<Entry>> list(const std::string& path);
    std::optional<std::vector<std::uint8_t>> read_file(const std::string& path);
    int write_file(const std::string& path, const std::vector<std::uint8_t>& data);   // create or replace
    int remove(const std::string& path);
    std::uint32_t free_clusters() const { return free_count_; }

private:
    struct Slot { std::uint32_t cluster; std::uint32_t index; };   // where a 32-byte entry lives
    struct Found { Entry e; std::vector<Slot> slots; };             // long-name slots, then the 8.3 slot

    std::uint32_t fat_get(std::uint32_t c);
    void fat_set(std::uint32_t c, std::uint32_t v);
    std::uint32_t alloc_cluster();
    void free_chain(std::uint32_t c);
    std::uint32_t cluster_lba(std::uint32_t c) const { return data_lba_ + (c - 2) * sec_per_clus_; }
    std::vector<Found> read_dir(std::uint32_t dir_cluster);
    std::optional<Entry> walk(const std::string& path, std::uint32_t* parent, std::string* last);
    void write_slot(const Slot& s, const std::uint8_t* entry32);
    std::vector<Slot> free_slots(std::uint32_t dir_cluster, std::size_t n);
    std::uint32_t write_chain(const std::vector<std::uint8_t>& data);
    void write_fsinfo();

    BlockDev& dev_;
    std::uint32_t bytes_per_sec_ = 0, sec_per_clus_ = 0, reserved_ = 0, num_fats_ = 0;
    std::uint32_t fat_size_ = 0, root_clus_ = 0, fsinfo_sec_ = 0, total_sec_ = 0;
    std::uint32_t data_lba_ = 0, cluster_count_ = 0, free_count_ = 0, next_free_ = 2;
};
