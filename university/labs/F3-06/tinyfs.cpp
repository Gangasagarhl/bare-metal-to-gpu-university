// tinyfs.cpp - the university's toy file system, kept in memory: a block bitmap, an inode table,
// directories, and a "crash" switch that stops a write part-way to show why order matters.
#include <array>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

constexpr int kBlocks = 16, kBlockSize = 16, kInodes = 8, kDirect = 4;

struct Inode {
    bool used = false;
    bool isDir = false;
    int size = 0;                      // bytes of data (files only)
    std::array<int, kDirect> block{};  // direct block numbers, -1 = none
};
struct Entry {
    std::string name;
    int inum;
};

struct Disk {
    std::array<bool, kBlocks> blockUsed{};
    std::array<std::string, kBlocks> data;  // block contents (not erased when freed)
    std::array<Inode, kInodes> inode;
    std::array<std::vector<Entry>, kInodes> dir;  // directory contents, by inode number
} disk;

int allocInode(bool isDir)
{
    for (int i = 0; i < kInodes; ++i) {
        if (!disk.inode[i].used) {
            disk.inode[i] = Inode{true, isDir, 0, {-1, -1, -1, -1}};
            return i;
        }
    }
    return -1;
}
int allocBlock()
{
    for (int b = 0; b < kBlocks; ++b) {
        if (!disk.blockUsed[b]) {
            disk.blockUsed[b] = true;
            return b;
        }
    }
    return -1;
}

// Path lookup: start at the root directory (inode 0), one name at a time.
int lookup(const std::string& path, bool verbose)
{
    int cur = 0;
    if (verbose) { std::printf("  '/' is inode 0\n"); }
    std::stringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '/')) {
        if (part.empty()) { continue; }
        int next = -1;
        for (const Entry& e : disk.dir[cur]) {
            if (e.name == part) { next = e.inum; }
        }
        if (verbose) {
            std::printf("  look up '%s' in directory inode %d -> %s%d\n", part.c_str(), cur,
                        next < 0 ? "not found " : "inode ", next);
        }
        if (next < 0) { return -1; }
        cur = next;
    }
    return cur;
}
int makeEntry(const std::string& path, bool isDir)
{
    const auto slash = path.rfind('/');
    const int parent = lookup(path.substr(0, slash), false);
    const int inum = allocInode(isDir);
    disk.dir[parent].push_back({path.substr(slash + 1), inum});
    return inum;
}

// Append one block of text. 'order' says which on-disk update comes second;
// 'steps' says how many of the three updates reach the disk before a crash.
void append(const std::string& path, const std::string& text, const std::string& order, int steps)
{
    Inode& ino = disk.inode[lookup(path, false)];
    const int b = allocBlock();  // update 1: bitmap
    std::printf("  step 1: bitmap marks block %d used\n", b);
    if (steps == 1) {
        std::printf("  CRASH\n");
        return;
    }
    int slot = 0;
    while (ino.block[slot] >= 0) { ++slot; }
    if (order == "data-first") {
        disk.data[b] = text;  // update 2: the data block
        std::printf("  step 2: data written into block %d\n", b);
        if (steps == 2) {
            std::printf("  CRASH\n");
            return;
        }
        ino.block[slot] = b;
        ino.size += static_cast<int>(text.size());  // update 3: the inode
        std::printf("  step 3: inode points to block %d, size %d\n", b, ino.size);
    } else {
        ino.block[slot] = b;
        ino.size += static_cast<int>(text.size());  // update 2: the inode
        std::printf("  step 2: inode points to block %d, size %d\n", b, ino.size);
        if (steps == 2) {
            std::printf("  CRASH\n");
            return;
        }
        disk.data[b] = text;  // update 3: the data block
        std::printf("  step 3: data written into block %d\n", b);
    }
}

void cat(const std::string& path)
{
    const Inode& ino = disk.inode[lookup(path, false)];
    std::string all;
    for (int b : ino.block) {
        if (b >= 0) { all += disk.data[b]; }
    }
    std::printf("  %s (%d bytes): \"%s\"\n", path.c_str(), ino.size,
                all.substr(0, static_cast<std::size_t>(ino.size)).c_str());
}

void removeFile(const std::string& path)
{
    const auto slash = path.rfind('/');
    const int parent = lookup(path.substr(0, slash), false);
    const int inum = lookup(path, false);
    for (int b : disk.inode[inum].block) {
        if (b >= 0) { disk.blockUsed[b] = false; }
    }  // data NOT erased
    disk.inode[inum].used = false;
    auto& entries = disk.dir[parent];
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].inum == inum) {
            entries.erase(entries.begin() + static_cast<long>(i));
            break;
        }
    }
}

// A tiny consistency checker in the spirit of fsck: compare the bitmap with the inodes.
void fsck()
{
    std::array<int, kBlocks> refs{};
    for (const Inode& ino : disk.inode) {
        if (!ino.used) { continue; }
        for (int b : ino.block) {
            if (b >= 0) { ++refs[b]; }
        }
    }
    int problems = 0;
    for (int b = 0; b < kBlocks; ++b) {
        if (disk.blockUsed[b] && refs[b] == 0) {
            std::printf("  block %d marked used but no inode points to it (leaked)\n", b);
            ++problems;
        }
        if (!disk.blockUsed[b] && refs[b] > 0) {
            std::printf("  block %d used by an inode but marked free (corrupt)\n", b);
            ++problems;
        }
    }
    std::printf("  fsck: %d problem(s) found\n", problems);
}

int main()
{
    disk.inode[0] = Inode{true, true, 0, {-1, -1, -1, -1}};  // the root directory
    std::string line;
    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        std::string cmd, path, arg, order;
        ss >> cmd >> path;
        if (cmd.empty() || cmd[0] == '#') { continue; }
        std::printf("> %s\n", line.c_str());
        if (cmd == "mkdir") {
            std::printf("  directory inode %d\n", makeEntry(path, true));
        } else if (cmd == "create") {
            std::printf("  file inode %d\n", makeEntry(path, false));
        } else if (cmd == "write") {
            ss >> arg;
            append(path, arg, "data-first", 3);
        } else if (cmd == "crashwrite") {
            int steps = 0;
            ss >> arg >> order >> steps;
            append(path, arg, order, steps);
        } else if (cmd == "cat") {
            cat(path);
        } else if (cmd == "rm") {
            removeFile(path);
            std::printf("  removed\n");
        } else if (cmd == "lookup") {
            std::printf("  -> inode %d\n", lookup(path, true));
        } else if (cmd == "fsck") {
            fsck();
        }
    }
    return 0;
}
