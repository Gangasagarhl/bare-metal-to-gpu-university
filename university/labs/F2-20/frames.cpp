#include <cstdint>
#include <iostream>
#include <memory>

int servedToday = 0;   // a global: lives for the whole program, outside stack and heap

std::uintptr_t addressOf(const void* p)
{
    return reinterpret_cast<std::uintptr_t>(p);
}

// Each call gets a new frame on the stack holding its own 'depth' copy and 'local'.
void cook(int depth, std::uintptr_t previous)
{
    int local = depth * 10;
    const std::uintptr_t here = addressOf(&local);
    std::cout << "depth " << depth << ": local at 0x" << std::hex << here << std::dec;
    if (previous != 0) {
        std::cout << "  (" << static_cast<long long>(here - previous) << " bytes from the caller's)";
    }
    std::cout << '\n';
    if (depth < 3) {
        cook(depth + 1, here);
    }
}

int main()
{
    cook(1, 0);
    auto a = std::make_unique<int>(1);   // heap
    auto b = std::make_unique<int>(2);   // heap
    std::cout << "heap int a at 0x" << std::hex << addressOf(a.get()) << '\n';
    std::cout << "heap int b at 0x" << addressOf(b.get()) << '\n';
    std::cout << "global servedToday at 0x" << addressOf(&servedToday) << std::dec << '\n';
    return 0;
}
