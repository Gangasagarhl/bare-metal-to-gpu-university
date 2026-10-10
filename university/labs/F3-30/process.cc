// process.cc - spawn (load an ELF executable), demand paging, exit, wait (B13).
#include "process.h"
#include "hooks.h"

extern "C" uint64_t boot_pml4[512];
extern "C" [[noreturn]] void enter_user(uint64_t rip, uint64_t rsp);   // syscall.S

namespace k {

namespace {
constexpr uint64_t kP = 1, kW = 2, kU = 4, kAddr = 0x000FFFFFFFFFF000ull;
Spinlock proc_lock{"process-table"};
WaitQueue kernel_children;      // the kernel waits here for processes it started
std::atomic<int> next_pid{1};
std::atomic<int> nlive{0};
Process* all = nullptr;         // the process table: every Process not yet reaped

uint64_t* table(uint64_t entry) { return reinterpret_cast<uint64_t*>(entry & kAddr); }

// Find (and create) the page-table entry for a user address in this address space.
uint64_t* user_pte(Process* p, uint64_t va, bool create)
{
    uint64_t* t = table(p->pml4);
    for (int shift = 39; shift > 12; shift -= 9) {
        uint64_t& e = t[(va >> shift) & 511];
        if (!(e & kP)) {
            if (!create) {
                return nullptr;
            }
            e = frame_alloc() | kP | kW | kU;   // user bit on every level, as the walk requires
        }
        t = table(e);
    }
    return &t[(va >> 12) & 511];
}

// Map one page of the program: zero-filled, then the file bytes that belong there.
void map_program_page(Process* p, uint64_t page)
{
    uint64_t frame = frame_alloc();   // already zeroed: covers .bss and the stack
    bool writable = page >= kUserStackTop - kUserStackSize && page < kUserStackTop;
    for (int i = 0; i < p->elf.nsegs; ++i) {
        const ElfSegment& s = p->elf.segs[i];
        uint64_t lo = s.vaddr > page ? s.vaddr : page;
        uint64_t hi = s.vaddr + s.memsz < page + kPage ? s.vaddr + s.memsz : page + kPage;
        if (lo >= hi) {
            continue;
        }
        writable = writable || (s.flags & 2);
        uint64_t file_end = s.vaddr + s.filesz;
        if (lo < file_end) {
            uint64_t n = (hi < file_end ? hi : file_end) - lo;
            memcpy(reinterpret_cast<void*>(frame + (lo - page)), p->image + s.offset + (lo - s.vaddr), n);
        }
    }
    *user_pte(p, page, true) = frame | kP | kU | (writable ? kW : 0);
    p->user_pages++;
}

bool page_belongs(Process* p, uint64_t page)
{
    if (page >= kUserStackTop - kUserStackSize && page < kUserStackTop) {
        return true;
    }
    for (int i = 0; i < p->elf.nsegs; ++i) {
        const ElfSegment& s = p->elf.segs[i];
        if (page + kPage > s.vaddr && page < s.vaddr + s.memsz) {
            return true;
        }
    }
    return false;
}

// Free every user page and every page table of the user half, then the PML4.
void free_address_space(Process* p)
{
    uint64_t* l4 = table(p->pml4);
    for (int i4 = 1; i4 < 512; ++i4) {   // entry 0 is the shared kernel half: not ours
        if (!(l4[i4] & kP)) continue;
        uint64_t* l3 = table(l4[i4]);
        for (int i3 = 0; i3 < 512; ++i3) {
            if (!(l3[i3] & kP)) continue;
            uint64_t* l2 = table(l3[i3]);
            for (int i2 = 0; i2 < 512; ++i2) {
                if (!(l2[i2] & kP)) continue;
                uint64_t* l1 = table(l2[i2]);
                for (int i1 = 0; i1 < 512; ++i1) {
                    if (l1[i1] & kP) frame_free(l1[i1] & kAddr);
                }
#ifndef BUG_LEAK_PAGE_TABLES
                frame_free(l2[i2] & kAddr);   // the page table itself
#endif
            }
#ifndef BUG_LEAK_PAGE_TABLES
            frame_free(l3[i3] & kAddr);       // the page directory
#endif
        }
#ifndef BUG_LEAK_PAGE_TABLES
        frame_free(l4[i4] & kAddr);           // the page-directory-pointer table
#endif
    }
    frame_free(p->pml4);
}

// Build argc, argv, envp and the auxiliary vector at the top of the user stack, as the
// System V AMD64 ABI "Process Initialization" section describes (title only).
uint64_t build_stack(Process* p, const char* const* argv, int argc)
{
    uint64_t page = kUserStackTop - kPage;
    map_program_page(p, page);
    auto* base = reinterpret_cast<uint8_t*>(*user_pte(p, page, false) & kAddr);   // kernel view
    uint64_t sp = kUserStackTop;
    uint64_t uargv[16];
    for (int i = argc - 1; i >= 0; --i) {   // the strings, at the very top
        uint64_t n = str_len(argv[i]) + 1;
        sp -= n;
        memcpy(base + (sp - page), argv[i], n);
        uargv[i] = sp;
    }
    const uint64_t aux[] = {3, p->elf.phdr_vaddr,   // AT_PHDR
                            4, 56,                    // AT_PHENT
                            5, p->elf.phnum,          // AT_PHNUM
                            6, kPage,                 // AT_PAGESZ
                            9, p->elf.entry,          // AT_ENTRY
                            0, 0};                    // AT_NULL
    uint64_t words = 1 + uint64_t(argc) + 1 + 1 + sizeof(aux) / 8;
    sp = (sp - words * 8) & ~uint64_t(15);   // rsp must be 16-byte aligned at _start
    auto* w = reinterpret_cast<uint64_t*>(base + (sp - page));
    int k = 0;
    w[k++] = uint64_t(argc);
    for (int i = 0; i < argc; ++i) {
        w[k++] = uargv[i];
    }
    w[k++] = 0;   // end of argv
    w[k++] = 0;   // envp: empty
    for (uint64_t v : aux) {
        w[k++] = v;
    }
    return sp;
}

void user_thread_start(void* arg)
{
    auto* p = static_cast<Process*>(arg);
    enter_user(p->elf.entry, p->initial_sp);
}

void reap(Process* p)   // with proc_lock held
{
    p->reaped = true;
    if (p->parent) {
        p->parent->children--;
    }
    for (Process** pp = &all; *pp != nullptr; pp = &(*pp)->next_all) {
        if (*pp == p) {
            *pp = p->next_all;
            break;
        }
    }
    kfree(p);
    nlive.fetch_sub(1);
}
}  // namespace

uint64_t kernel_cr3() { return reinterpret_cast<uint64_t>(boot_pml4); }
uint64_t proc_cr3(Process* p) { return p->pml4; }
Process* current_process() { return this_cpu().current->proc; }
int processes_live() { return nlive.load(); }

int64_t process_spawn(const uint8_t* image, uint64_t size, const char* name,
                      const char* const* argv, int argc, Process* parent)
{
    thread_reap_zombies();   // here, with interrupts on (thread_create below runs with them off)
    ElfImage elf;
    ElfError e = elf_check(image, size, kUserBase, kUserStackTop - kUserStackSize, elf);
    if (e != ElfError::None) {
        if (!console_quiet()) {
            kprintf("exec %s: rejected: %s\n", name, elf_error_name(e));
        }
        return kErrNoExec;
    }
    if (argc < 0 || argc > 15) {
        return kErrInval;
    }
    auto* p = static_cast<Process*>(kmalloc(sizeof(Process)));
    p->pid = next_pid.fetch_add(1);
    str_copy(p->name, name, sizeof(p->name));
    p->parent = parent;
    p->elf = elf;
    p->image = image;
    p->image_size = size;
    p->pml4 = frame_alloc();
    table(p->pml4)[0] = boot_pml4[0];   // the kernel half: shared, supervisor-only
    p->initial_sp = build_stack(p, argv, argc);
    nlive.fetch_add(1);
    uint64_t f = proc_lock.lock_irqsave();
    if (parent) {
        parent->children++;
    }
    p->next_all = all;
    all = p;
    int64_t pid = p->pid;   // p may already be gone when thread_create returns
    p->thread = thread_create(p->name, user_thread_start, p, -1, p);
    proc_lock.unlock_irqrestore(f);
    return pid;
}

int64_t process_spawn_module(const char* module, const char* const* argv, int argc,
                             Process* parent)
{
    const BootModule* m = boot_module_find(module);
    if (m == nullptr) {
        return kErrNoEnt;
    }
    return process_spawn(m->data, m->size, m->name, argv, argc, parent);
}

void process_exit(int status)
{
    Thread* t = this_cpu().current;
    Process* p = t->proc;
    irq_disable();
    write_cr3(kernel_cr3());   // stop using the address space we are about to free
    t->proc = nullptr;
    irq_enable();
    free_address_space(p);
    uint64_t f = proc_lock.lock_irqsave();
    for (Process* c = all; c != nullptr;) {   // our children: reap the dead, orphan the living
        Process* next = c->next_all;
        if (c->parent == p) {
            if (c->exited) {
                reap(c);
            } else {
                c->parent = nullptr;
                c->orphan = true;
                p->children--;
            }
        }
        c = next;
    }
    p->status = status;
    p->exited = true;
    p->thread = nullptr;
    if (p->orphan) {
        reap(p);   // nobody will wait for it
    } else {
        (p->parent ? p->parent->child_exited : kernel_children).wake_all();
    }
    proc_lock.unlock_irqrestore(f);
    thread_exit();
}

bool process_try_wait(Process* parent, int* status)
{
    return process_wait(parent, status, true) > 0;
}

int64_t process_wait(Process* parent, int* status, bool nohang)
{
    uint64_t f = proc_lock.lock_irqsave();
    WaitQueue& wq = parent ? parent->child_exited : kernel_children;
    for (;;) {
        Process* found = nullptr;
        for (Process* p = all; p != nullptr; p = p->next_all) {
            if (p->parent == parent && !p->orphan && p->exited) {
                found = p;
                break;
            }
        }
        if (found) {
            int64_t pid = found->pid;
            *status = found->status;
            reap(found);
            proc_lock.unlock_irqrestore(f);
            return pid;
        }
        bool any = false;
        for (Process* p = all; p != nullptr && !any; p = p->next_all) {
            any = p->parent == parent && !p->orphan;
        }
        if (!any || nohang) {
            proc_lock.unlock_irqrestore(f);
            return any ? 0 : kErrChild;
        }
        wq.sleep(proc_lock);
    }
}

void process_kill(int pid)
{
    uint64_t f = proc_lock.lock_irqsave();
    for (Process* p = all; p != nullptr; p = p->next_all) {
        if (p->pid == pid && !p->exited && p->thread) {
            p->thread->kill_requested = true;   // acted on before it next returns to ring 3
        }
    }
    proc_lock.unlock_irqrestore(f);
}

// ---------------------------------------------------------------- page faults
// Demand paging: a fault on a user page that belongs to a segment or the stack, and is
// not mapped yet, is answered by mapping it. Everything else is a real fault.
bool vm_handle_fault(TrapFrame* f, uint64_t cr2)
{
    Process* p = current_process();
    if (p == nullptr || (f->error & 1) || cr2 < kUserBase || cr2 >= kUserTop) {
        return false;   // no process, a protection violation, or not a user address
    }
    uint64_t page = cr2 & ~(kPage - 1);
    if (!page_belongs(p, page)) {
        return false;
    }
    map_program_page(p, page);
    return true;
}

void user_fault(TrapFrame* f, uint64_t cr2)
{
    static const char* const names[] = {"divide error", "debug", "NMI", "breakpoint",
        "overflow", "bound range", "invalid opcode", "device not available", "double fault",
        "coprocessor", "invalid TSS", "segment not present", "stack fault",
        "general protection", "page fault"};
    Process* p = current_process();
    kprintf("USER FAULT: pid %d '%s': %s (vector %lu) at rip=%016lx", p ? p->pid : -1,
            p ? p->name : "?", f->vector < 15 ? names[f->vector] : "exception", f->vector, f->rip);
    if (f->vector == 14) {
        kprintf(", address %016lx (%s %s)", cr2, (f->error & 1) ? "protection" : "not mapped",
                (f->error & 2) ? "write" : "read");
    }
    kprintf(": process killed, the kernel continues\n");
    process_exit(kStatusKilled | int(f->vector));
}

void user_return_check(TrapFrame*)
{
    Thread* t = this_cpu().current;
    if (t->kill_requested && t->proc) {
        irq_enable();
        process_exit(kStatusKilledByKernel);
    }
}

}  // namespace k
