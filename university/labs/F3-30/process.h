// process.h - processes: an address space, one user thread, exit status, parent/child.
#pragma once
#include "elf_check.h"
#include "thread.h"
#include "uaccess.h"

namespace k {

// System-call numbers and error codes of the OS303 kernel. They are this kernel's own
// ABI (documented in F3-29), not Linux's numbers.
constexpr uint64_t kSysExit = 0, kSysWrite = 1, kSysGetpid = 2, kSysSpawn = 3, kSysWait = 4,
                   kSysYield = 5;
constexpr int64_t kErrFault = -2, kErrBadFd = -3, kErrNoSys = -4, kErrNoEnt = -5,
                  kErrNoExec = -6, kErrChild = -7, kErrInval = -8;

constexpr uint64_t kUserStackTop = kUserTop - kPage;         // one unmapped page above
constexpr uint64_t kUserStackSize = 64 * 1024;
constexpr int kStatusKilled = 0x100;                           // | exception vector
constexpr int kStatusKilledByKernel = 0x1FF;

struct Process {
    int pid;
    char name[32];
    Process* parent;
    uint64_t pml4;              // physical address of the top page table
    ElfImage elf;
    const uint8_t* image;       // the ELF file in memory (a boot module)
    uint64_t image_size;
    Thread* thread;
    bool exited, reaped;
    int status;
    int children;               // not yet reaped
    bool orphan;                // parent exited first: reap at exit
    uint64_t initial_sp;        // user stack pointer for the first entry to ring 3
    uint64_t user_pages;        // data pages mapped
    WaitQueue child_exited;
    Process* next_all;          // list of all processes (process table)
};

// Load the ELF file 'image' and start it with argv; returns the new pid or an Err.
int64_t process_spawn(const uint8_t* image, uint64_t size, const char* name,
                      const char* const* argv, int argc, Process* parent);
int64_t process_spawn_module(const char* module, const char* const* argv, int argc,
                             Process* parent);
// Wait for any child of 'parent' (nullptr: children of the kernel). Returns pid or Err.
int64_t process_wait(Process* parent, int* status, bool nohang = false);
bool process_try_wait(Process* parent, int* status);   // true if a child was reaped
[[noreturn]] void process_exit(int status);
Process* current_process();
int processes_live();
void process_kill(int pid);

}  // namespace k
