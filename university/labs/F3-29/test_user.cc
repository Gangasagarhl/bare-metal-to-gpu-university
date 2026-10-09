// test_user.cc - the B12 acceptance tests: user programs run in ring 3, call the
// kernel through SYSCALL, and cannot crash or read the kernel.
#include "hooks.h"
#include "process.h"

namespace k {

namespace {
int run(const char* module)   // start a boot module as a process, wait, return its status
{
    const char* argv[] = {module};
    int64_t pid = process_spawn_module(module, argv, 1, nullptr);
    if (pid < 0) {
        kprintf("spawn %s failed: %ld\n", module, pid);
        return -1;
    }
    int status = -1;
    process_wait(nullptr, &status);
    kprintf("kernel: process %ld '%s' exited with status 0x%x\n", pid, module, status);
    return status;
}
}  // namespace

bool test_user()
{
    kprintf("boot modules:");
    for (int i = 0; i < boot_module_count(); ++i) {
        kprintf(" %s (%lu bytes)", boot_module(i)->name, boot_module(i)->size);
    }
    kprintf("\n");
    uint64_t frames0 = frames_free();
    bool ok = run("u_hello.elf") == 42;
    ok = run("u_badptr.elf") == 0 && ok;                                // every bad call got an error
    ok = run("u_priv.elf") == (kStatusKilled | 13) && ok;               // general protection
    ok = run("u_peek.elf") == (kStatusKilled | 14) && ok;               // page fault, U/S bit
    thread_reap_zombies();
    kprintf("kernel: still running after the faults; free frames %lu before, %lu after\n",
            frames0, frames_free());
    return ok && frames_free() == frames0;
}

}  // namespace k
