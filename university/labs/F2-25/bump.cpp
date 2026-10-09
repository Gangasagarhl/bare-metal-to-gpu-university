#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>

// A bump allocator: hands out pieces of one fixed arena by moving a cursor forward.
// It never frees single pieces; reset() makes the whole arena free again.
class BumpArena
{
public:
    explicit BumpArena(std::span<std::byte> memory) : memory_(memory) {}

    void* allocate(std::size_t size, std::size_t align)
    {
        assert(align != 0 && (align & (align - 1)) == 0);     // a power of two
        const auto base = reinterpret_cast<std::uintptr_t>(memory_.data());
        const std::uintptr_t current = base + used_;
        const std::uintptr_t aligned = (current + align - 1) & ~(std::uintptr_t{align} - 1);
        const std::size_t padding = aligned - current;
        if (padding > memory_.size() - used_ || size > memory_.size() - used_ - padding) {
            return nullptr;                                   // does not fit: refuse, never overrun
        }
        used_ += padding + size;
        return memory_.data() + (aligned - base);
    }

    void reset() { used_ = 0; }
    std::size_t used() const { return used_; }

private:
    std::span<std::byte> memory_;
    std::size_t used_ = 0;
};

int main()
{
    alignas(16) std::array<std::byte, 64> arena{};
    BumpArena bump(arena);

    void* a = bump.allocate(1, 1);
    void* b = bump.allocate(8, 8);
    void* c = bump.allocate(4, 4);
    std::cout << "a at offset " << static_cast<std::byte*>(a) - arena.data() << '\n';
    std::cout << "b at offset " << static_cast<std::byte*>(b) - arena.data() << " (aligned to 8)\n";
    std::cout << "c at offset " << static_cast<std::byte*>(c) - arena.data() << '\n';
    assert(reinterpret_cast<std::uintptr_t>(b) % 8 == 0);
    std::cout << "used " << bump.used() << " of " << arena.size() << " bytes\n";

    void* big = bump.allocate(100, 1);                         // more than is left
    assert(big == nullptr);
    std::cout << "a 100-byte request is refused: " << (big == nullptr ? "yes" : "no") << '\n';

    void* rest = bump.allocate(64 - bump.used(), 1);            // exactly what is left
    assert(rest != nullptr && bump.used() == 64);
    assert(bump.allocate(1, 1) == nullptr);                     // full
    bump.reset();
    assert(bump.allocate(64, 16) == arena.data());              // whole arena again
    std::cout << "all tests passed\n";
    return 0;
}
