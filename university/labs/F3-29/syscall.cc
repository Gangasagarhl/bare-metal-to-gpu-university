// syscall.cc - MSR setup for SYSCALL/SYSRET, SMEP/SMAP, safe user copies and the
// system-call table (milestone B12). MSR numbers and bits as used here: Intel SDM
// Vol. 4 (STAR, LSTAR, FMASK, EFER) and Vol. 3 (SMEP, SMAP); titles only.
#include "hooks.h"
#include "process.h"

extern "C" void syscall_entry();
extern "C" uint64_t uaccess_copy(void* dst, const void* src, uint64_t n);
extern "C" uint64_t __uaccess_fixups_start[], __uaccess_fixups_end[];

namespace k {

namespace {
bool smap_on = false;

struct SyscallFrame {   // pushed by syscall.S, lowest address first
    uint64_t r9, r8, r10, rdx, rsi, rdi, nr, rip, rflags, rsp;
};
}  // namespace

void syscall_init_cpu()
{
    wrmsr(0xC0000080, rdmsr(0xC0000080) | 1);   // EFER.SCE: enable SYSCALL/SYSRET
    // STAR[47:32]: SYSCALL loads CS = 0x08, SS = 0x10.
    // STAR[63:48]: SYSRET loads SS = 0x10 + 8 = 0x18 and CS = 0x10 + 16 = 0x20 (RPL 3).
    wrmsr(0xC0000081, (uint64_t(0x10) << 48) | (uint64_t(kKernelCS) << 32));
    wrmsr(0xC0000082, reinterpret_cast<uint64_t>(syscall_entry));   // LSTAR: entry point
    // FMASK: clear IF (no interrupts until we are on the kernel stack), TF, DF and AC
    // (AC = 1 would switch SMAP off for the kernel).
    wrmsr(0xC0000084, (1u << 9) | (1u << 8) | (1u << 10) | (1u << 18));
    uint32_t r[4];
    cpuid(7, 0, r);
    bool smep = (r[1] >> 7) & 1, smap = (r[1] >> 20) & 1;
    if (boot_arg("nosmap") != nullptr) {
        smep = smap = false;
    }
    write_cr4(read_cr4() | (smep ? 1u << 20 : 0) | (smap ? 1u << 21 : 0));
    smap_on = smap;
    if (this_cpu().id == 0) {
        kprintf("syscall: SYSCALL/SYSRET enabled; SMEP %s, SMAP %s\n", smep ? "on" : "off",
                smap ? "on" : "off");
    }
}

// Copies that check the range first and survive a fault in the middle. They return
// 0 or kErrFault. STAC/CLAC open and close the SMAP window around the copy only.
int64_t copy_from_user(void* dst, uint64_t usrc, uint64_t n)
{
    if (!user_range_ok(usrc, n)) {
        return kErrFault;
    }
    if (smap_on) asm volatile("stac" ::: "memory");
    uint64_t left = uaccess_copy(dst, reinterpret_cast<const void*>(usrc), n);
    if (smap_on) asm volatile("clac" ::: "memory");
    return left == 0 ? 0 : kErrFault;
}

int64_t copy_to_user(uint64_t udst, const void* src, uint64_t n)
{
    if (!user_range_ok(udst, n)) {
        return kErrFault;
    }
    if (smap_on) asm volatile("stac" ::: "memory");
    uint64_t left = uaccess_copy(reinterpret_cast<void*>(udst), src, n);
    if (smap_on) asm volatile("clac" ::: "memory");
    return left == 0 ? 0 : kErrFault;
}

// Copy a NUL-terminated string of at most cap - 1 characters.
int64_t copy_string_from_user(char* dst, uint64_t usrc, uint64_t cap)
{
    for (uint64_t i = 0; i < cap; ++i) {
        if (copy_from_user(&dst[i], usrc + i, 1) != 0) {
            return kErrFault;
        }
        if (dst[i] == '\0') {
            return 0;
        }
    }
    return kErrInval;
}

bool uaccess_fixup(TrapFrame* f)   // called by the page-fault handler for kernel faults
{
    for (uint64_t* e = __uaccess_fixups_start; e < __uaccess_fixups_end; e += 2) {
        if (f->rip == e[0]) {
            f->rip = e[1];   // continue at the fixup: the copy reports failure
            return true;
        }
    }
    return false;
}

namespace {
// noinline: so that a crash inside it shows up as sys_write in a backtrace
__attribute__((noinline)) int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len)
{
    if (fd != 1 && fd != 2) {
        return kErrBadFd;
    }
#ifdef BUG_TRUST_USER_POINTER
    // The forensic build: "the pointer comes from our own C library, it is fine".
    console_write_raw(reinterpret_cast<const char*>(buf), len);
    return int64_t(len);
#else
    if (!user_range_ok(buf, len)) {
        return kErrFault;
    }
    char chunk[256];
    for (uint64_t done = 0; done < len;) {
        uint64_t n = len - done < sizeof(chunk) ? len - done : sizeof(chunk);
        if (copy_from_user(chunk, buf + done, n) != 0) {
            return kErrFault;
        }
        if (!console_quiet()) {
            console_write_raw(chunk, n);
        }
        done += n;
    }
    return int64_t(len);
#endif
}

int64_t sys_spawn(uint64_t upath, uint64_t uargv)   // F3-30
{
    char path[32];
    char args[8][64];
    const char* argv[8];
    if (copy_string_from_user(path, upath, sizeof(path)) != 0) {
        return kErrFault;
    }
    int argc = 0;
    for (; argc < 8; ++argc) {
        uint64_t p;
        if (copy_from_user(&p, uargv + 8 * uint64_t(argc), 8) != 0) {
            return kErrFault;
        }
        if (p == 0) {
            break;
        }
        if (copy_string_from_user(args[argc], p, sizeof(args[argc])) != 0) {
            return kErrFault;
        }
        argv[argc] = args[argc];
    }
    return process_spawn_module(path, argv, argc, current_process());
}

int64_t sys_wait(uint64_t ustatus)   // F3-30
{
    int status = 0;
    int64_t pid = process_wait(current_process(), &status);
    if (pid > 0 && ustatus != 0 && copy_to_user(ustatus, &status, sizeof(status)) != 0) {
        return kErrFault;
    }
    return pid;
}
}  // namespace

extern "C" int64_t syscall_dispatch(SyscallFrame* f)
{
    int64_t r;
    switch (f->nr) {
    case kSysExit:   process_exit(int(f->rdi & 0xFF));
    case kSysWrite:  r = sys_write(f->rdi, f->rsi, f->rdx); break;
    case kSysGetpid: r = current_process()->pid; break;
    case kSysSpawn:  r = sys_spawn(f->rdi, f->rsi); break;
    case kSysWait:   r = sys_wait(f->rdi); break;
    case kSysYield:  thread_yield(); r = 0; break;
    default:         r = kErrNoSys;
    }
    if (this_cpu().current->kill_requested) {
        process_exit(kStatusKilledByKernel);
    }
    return r;
}

}  // namespace k
