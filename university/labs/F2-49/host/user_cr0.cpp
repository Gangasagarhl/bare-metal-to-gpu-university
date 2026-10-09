// user_cr0.cpp: reading control register CR0 from user mode (privilege level 3).
#include <cstdint>
#include <cstdio>

int main()
{
    std::printf("about to read CR0 from user mode\n");
    std::fflush(stdout);
    std::uint64_t cr0 = 0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    std::printf("CR0 = 0x%llx\n", static_cast<unsigned long long>(cr0));
    return 0;
}
