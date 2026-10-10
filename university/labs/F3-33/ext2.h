// ext2.h - F3-33: a small ext2 driver (read; create, replace, delete files; make directories).
// Host C++ over a disk image; in the kernel the Disk is the B14 block cache.
#pragma once
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

class Disk                               // whole blocks; can simulate a power cut
{
public:
    Disk(const char* path, std::uint32_t block_size);
    ~Disk();
    Disk(const Disk&) = delete;
    Disk& operator=(const Disk&) = delete;
    bool ok() const { return f_ != nullptr; }
    void set_block_size(std::uint32_t bs) { bs_ = bs; }
    void read(std::uint32_t b, std::uint8_t* buf);
    void write(std::uint32_t b, const std::uint8_t* buf);
    void read_bytes(std::uint64_t off, std::uint8_t* buf, std::uint32_t n);
    long cut_after = -1;                 // >= 0: writes after this many are lost (power cut)
    struct Write { std::uint32_t block; bool reached; };
    std::vector<Write> log;              // every write the driver issued, in order
    std::size_t writes = 0, lost = 0;
private:
    std::FILE* f_;
    std::uint32_t bs_;
};

struct DirEntry { std::uint32_t inode; std::uint8_t type; std::string name; };

struct Inode                             // the fields this driver uses (128-byte inode)
{
    std::uint16_t mode = 0;
    std::uint32_t size = 0, dtime = 0, blocks512 = 0;
    std::uint16_t links = 0;
    std::uint32_t block[15] = {};        // 12 direct, single, double, triple indirect
    bool is_dir() const { return (mode & 0xF000) == 0x4000; }
};

class Ext2
{
public:
    explicit Ext2(Disk& d) : d_(d) {}
    bool mount();
    void print_info() const;
    std::optional<std::vector<DirEntry>> list(const std::string& path);
    std::optional<std::vector<std::uint8_t>> read_file(const std::string& path);
    int write_file(const std::string& path, const std::vector<std::uint8_t>& data);  // create or replace
    int mkdir(const std::string& path);
    int unlink(const std::string& path);
    std::string role(std::uint32_t block) const;      // what a block is used for (for write logs)
    std::uint32_t free_blocks() const { return free_blocks_; }
    std::uint32_t free_inodes() const { return free_inodes_; }

private:
    struct Group { std::uint32_t block_bitmap, inode_bitmap, inode_table; std::uint16_t free_blocks, free_inodes, used_dirs; };

    Inode read_inode(std::uint32_t ino);
    void write_inode(std::uint32_t ino, const Inode& in);
    std::uint32_t bmap(const Inode& in, std::uint32_t logical);
    std::vector<std::uint32_t> all_blocks(const Inode& in);          // data and indirect blocks
    std::optional<std::uint32_t> lookup(const std::string& path, std::uint32_t* parent, std::string* last);
    std::vector<DirEntry> read_dir(std::uint32_t ino);
    std::uint32_t alloc_inode(bool dir);
    void free_inode(std::uint32_t ino, bool dir);
    std::vector<std::uint32_t> pick_blocks(std::uint32_t n);         // choose; bitmap not yet changed
    void mark_blocks(const std::vector<std::uint32_t>& blocks, bool used);
    std::uint32_t layout(Inode& in, const std::vector<std::uint32_t>& blocks, std::uint32_t data_blocks,
                         const std::vector<std::uint8_t>& data);     // write data and indirect blocks
    int add_entry(std::uint32_t dir, const std::string& name, std::uint32_t ino, std::uint8_t type);
    int remove_entry(std::uint32_t dir, const std::string& name);
    void write_group(std::uint32_t g);
    void write_super();
    static std::uint32_t blocks_needed(std::uint32_t data_blocks, std::uint32_t ptrs);

    Disk& d_;
    std::uint32_t bs_ = 1024, inodes_count_ = 0, blocks_count_ = 0, first_data_ = 0;
    std::uint32_t blocks_per_group_ = 0, inodes_per_group_ = 0, inode_size_ = 128, first_ino_ = 11;
    std::uint32_t free_blocks_ = 0, free_inodes_ = 0, ngroups_ = 0;
    std::vector<Group> groups_;
    std::uint8_t sb_[1024] = {};
};
