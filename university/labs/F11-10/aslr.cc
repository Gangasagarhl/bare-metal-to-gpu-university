// aslr.cc - F11-10: ASLR (address space layout randomisation). The program
// prints the address of a stack variable, a heap allocation and a function.
// run.sh runs it several times; with ASLR on, the addresses differ between runs,
// so an attacker cannot know in advance where the stack, the heap or the code
// will be. Addresses are printed, never interpreted as a fixed number in the
// chapter (they are different on every machine and every run).
#include <cstdio>
#include <cstdint>
#include <memory>

static int code_marker() { return 0; }

int main()
{
    int on_stack = 0;
    auto on_heap = std::make_unique<int>(0);
    std::printf("stack %#018zx  heap %#018zx  code %#018zx\n",
                reinterpret_cast<std::uintptr_t>(&on_stack),
                reinterpret_cast<std::uintptr_t>(on_heap.get()),
                reinterpret_cast<std::uintptr_t>(reinterpret_cast<void*>(&code_marker)));
    return 0;
}
