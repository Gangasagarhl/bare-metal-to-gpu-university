#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <span>
#include <vector>

// Reference solution for the SP201 course project (free-list half). Not shown on the
// learner page: it proves the project is feasible with the course toolchain (guide 11.5).
// First-fit free list inside one arena; every block starts with a header; freeing merges
// neighbouring free blocks (coalescing).
class FreeListArena
{
public:
    explicit FreeListArena(std::span<std::byte> memory) : memory_(memory)
    {
        assert(memory.size() >= sizeof(Header) * 2);
        assert(reinterpret_cast<std::uintptr_t>(memory.data()) % kAlign == 0);
        auto* first = header(0);
        first->size = memory.size() - sizeof(Header);
        first->free = true;
    }

    void* allocate(std::size_t size)
    {
        if (size == 0) {
            size = 1;
        }
        size = roundUp(size);
        for (std::size_t off = 0; off < memory_.size(); off += sizeof(Header) + header(off)->size) {
            Header* h = header(off);
            if (!h->free || h->size < size) {
                continue;
            }
            if (h->size >= size + sizeof(Header) + kAlign) {    // split: the rest stays free
                Header* rest = header(off + sizeof(Header) + size);
                rest->size = h->size - size - sizeof(Header);
                rest->free = true;
                h->size = size;
            }
            h->free = false;
            return memory_.data() + off + sizeof(Header);
        }
        return nullptr;
    }

    void deallocate(void* p)
    {
        if (p == nullptr) {
            return;
        }
        const auto off = static_cast<std::size_t>(static_cast<std::byte*>(p) - memory_.data()) - sizeof(Header);
        assert(!header(off)->free && "double free");
        header(off)->free = true;
        coalesce();
    }

    std::size_t freeBytes() const
    {
        std::size_t total = 0;
        for (std::size_t off = 0; off < memory_.size(); off += sizeof(Header) + header(off)->size) {
            if (header(off)->free) {
                total += header(off)->size;
            }
        }
        return total;
    }

private:
    struct Header
    {
        std::size_t size;   // payload bytes after this header
        bool free;
    };
    static constexpr std::size_t kAlign = alignof(std::max_align_t);
    static_assert(sizeof(Header) % kAlign == 0, "headers keep payloads aligned");

    static std::size_t roundUp(std::size_t n) { return (n + kAlign - 1) & ~(kAlign - 1); }
    Header* header(std::size_t off) const { return reinterpret_cast<Header*>(memory_.data() + off); }

    void coalesce()
    {
        std::size_t off = 0;
        while (off < memory_.size()) {
            Header* h = header(off);
            const std::size_t next = off + sizeof(Header) + h->size;
            if (h->free && next < memory_.size() && header(next)->free) {
                h->size += sizeof(Header) + header(next)->size;     // absorb the neighbour
                continue;
            }
            off = next;
        }
    }

    std::span<std::byte> memory_;
};

int main()
{
    alignas(std::max_align_t) static std::array<std::byte, 4096> arena{};
    FreeListArena heap(arena);
    const std::size_t initialFree = heap.freeBytes();

    std::mt19937 rng(2026);
    struct Live { std::byte* p; std::size_t n; unsigned char tag; };
    std::vector<Live> live;
    int refused = 0;
    for (int step = 0; step < 20000; ++step) {
        if (live.empty() || rng() % 2 == 0) {
            const std::size_t n = 1 + rng() % 200;
            auto* p = static_cast<std::byte*>(heap.allocate(n));
            if (p == nullptr) {
                ++refused;
                continue;
            }
            assert(reinterpret_cast<std::uintptr_t>(p) % alignof(std::max_align_t) == 0);
            for (const Live& l : live) {                         // no two live blocks overlap
                assert(p + n <= l.p || l.p + l.n <= p);
            }
            const auto tag = static_cast<unsigned char>(step);
            std::memset(p, tag, n);
            live.push_back({p, n, tag});
        } else {
            const std::size_t i = rng() % live.size();
            for (std::size_t k = 0; k < live[i].n; ++k) {        // contents survived untouched
                assert(live[i].p[k] == std::byte{live[i].tag});
            }
            heap.deallocate(live[i].p);
            live.erase(live.begin() + static_cast<std::ptrdiff_t>(i));
        }
    }
    for (const Live& l : live) {
        heap.deallocate(l.p);
    }
    assert(heap.freeBytes() == initialFree);                     // everything came back
    std::cout << "20000 random steps, " << refused << " requests refused when full\n";
    std::cout << "free bytes at start " << initialFree << ", at end " << heap.freeBytes() << '\n';
    std::cout << "all tests passed\n";
    return 0;
}
