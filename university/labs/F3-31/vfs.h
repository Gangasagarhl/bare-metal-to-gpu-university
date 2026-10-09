// vfs.h - F3-31 mini kernel: the virtual file system interface.
// Every file system fills in a VnodeOps table; the VFS never looks inside a file system.
#pragma once
#include "kbase.h"

enum class VType : uint8_t { File, Dir };

struct Vnode;
struct DirEnt { char name[100]; VType type; };

struct VnodeOps {                        // one table per file-system type
    int (*lookup)(Vnode* dir, const char* name, Vnode** out);      // 0 or -E_...
    long (*read)(Vnode* vn, uint32_t off, void* buf, uint32_t n);   // bytes read, 0 at end
    int (*readdir)(Vnode* dir, uint32_t index, DirEnt* out);        // 1 = entry, 0 = end
};

struct Vnode {                           // the VFS's view of one file or directory
    const VnodeOps* ops;
    VType type;
    uint32_t mode;                       // permission bits as stored by the file system
    uint32_t size;
    uint32_t ino;                        // number unique inside its file system
    uint32_t dev;                        // which mounted file system (index in mount table)
    void* priv;                          // the file system's own node
    Vnode* mounted;                      // root of a file system mounted on this directory
};

struct Stat { VType type; uint32_t mode, size, ino, dev; };

int vfs_mount(const char* path, Vnode* root, const char* fs_name);
int vfs_lookup(const char* path, Vnode** out);
int vfs_open(const char* path);          // file descriptor >= 0, or -E_...
long vfs_read(int fd, void* buf, uint32_t n);
long vfs_seek(int fd, long off);         // absolute position only (SEEK_SET)
int vfs_readdir(int fd, DirEnt* out);    // 1 = entry, 0 = end, or -E_...
int vfs_stat(const char* path, Stat* st);
int vfs_dup(int fd);
int vfs_close(int fd);
int vfs_mount_count();
const char* vfs_mount_info(int i, const char** fs_name);

// File systems of this kernel
Vnode* tarfs_create(const uint8_t* image, uint32_t len, uint32_t dev);
void tarfs_stats(uint32_t* files, uint32_t* dirs, uint32_t* skipped, uint32_t* bad_checksum);
Vnode* infofs_create(uint32_t dev);
