// ustar.h - DR301 F4-07 (project): a read-only USTAR archive as the root file system.
// The archive format is the POSIX "ustar" interchange format: 512-byte headers, octal
// sizes, data padded to 512 bytes, two zero blocks at the end.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace ustar {
// Reads 'bytes' (a multiple of the device block size, 4 KiB aligned buffer) at byte offset 'off'.
using ReadFn = int (*)(uint64_t off, void* buf, uint32_t bytes);
struct Entry {
    char path[256];
    char type;                   // '0' file, '5' directory
    uint64_t size;
    uint64_t data_off;           // byte offset of the data in the volume
};
// Calls fn for every entry; returns the number of entries, or a negative error
// (-1 bad checksum, -2 read error).
int walk(ReadFn read, void (*fn)(const Entry& e));
// FNV-1a of a file's data, read in 64 KiB pieces.
int hash(ReadFn read, const Entry& e, uint32_t& out);
// Copy at most 'max' bytes of a file into 'dst'.
int read_file(ReadFn read, const Entry& e, char* dst, uint32_t max);
}
