// heap.h - F3-22: a size-class slab allocator plus a page-based path for large blocks.
// Small blocks (up to 2048 bytes) come from 16 KiB slabs, one list of slabs per size class
// ("cache"); a slab's header sits at its start, so free() finds it by rounding the pointer
// down to 16 KiB. Freed objects are poisoned (filled with 0xDE) and checked when reused.
// Large blocks get whole pages from the environment, with a header page in front.
// The environment (kernel or host test) supplies the memory through four functions.
// Ideas after Bonwick's slab allocator paper (pending verification, F3-22 D1).
#pragma once
#include <cstddef>
#include <cstdint>

struct HeapEnv {
    void* (*get_slab)();                       // 16 KiB, aligned to 16 KiB
    void (*put_slab)(void*);
    void* (*get_pages)(size_t pages);          // 'pages' consecutive 4 KiB pages
    void (*put_pages)(void*, size_t pages);
    bool (*is_large)(const void*);             // true for pointers from get_pages
    void (*report)(const char* what, const char* cache, const void* obj, size_t offset, uint8_t value);
};

class Heap {
public:
    static constexpr size_t kSlabSize = 16384;
    static constexpr size_t kPage = 4096;
    static constexpr int kClasses = 8;         // 16, 32, ..., 2048
    static constexpr uint8_t kPoison = 0xDE;

    struct CacheStats {
        const char* name;
        size_t size;
        uint64_t live;
        uint64_t slabs;
    };

    void init(const HeapEnv& env)
    {
        env_ = env;
        static const char* const kNames[kClasses] = {"kmalloc-16", "kmalloc-32", "kmalloc-64", "kmalloc-128",
                                                     "kmalloc-256", "kmalloc-512", "kmalloc-1024", "kmalloc-2048"};
        for (int i = 0; i < kClasses; ++i) {
            caches_[i] = Cache{kNames[i], size_t{16} << i, nullptr, 0, 0};
        }
        large_live_ = 0;
        large_pages_ = 0;
    }

    // align must be a power of two; up to 2048 small blocks are naturally aligned to their size
    // class, large blocks to a page. Larger alignments are refused (nullptr).
    void* alloc(size_t size, size_t align = 16)
    {
        if (size == 0) {
            size = 1;
        }
        if (align > kPage || (align & (align - 1)) != 0) {
            return nullptr;
        }
        size_t need = size > align ? size : align;
        if (need > (size_t{16} << (kClasses - 1))) {
            return alloc_large(size);
        }
        int c = 0;
        while ((size_t{16} << c) < need) {
            ++c;
        }
        return alloc_small(caches_[c]);
    }

    void free(void* p)
    {
        if (p == nullptr) {
            return;
        }
        if (env_.is_large(p)) {
            free_large(p);
            return;
        }
        Slab* s = slab_of(p);
        if (s->magic != kSlabMagic) {
            env_.report("free of a pointer that is not from this heap", "?", p, 0, 0);
            return;
        }
        Cache& c = *s->cache;
        auto* bytes = static_cast<uint8_t*>(p);
        if (bytes[c.size - 1] == kPoison && is_on_free_list(s, p)) {
            env_.report("double free", c.name, p, 0, 0);
            return;
        }
        for (size_t i = sizeof(void*); i < c.size; ++i) {
            bytes[i] = kPoison;                // poison everything after the list link
        }
        *static_cast<void**>(p) = s->free_list;
        s->free_list = p;
        --s->in_use;
        --c.live;
        if (s->in_use == 0 && c.slabs > 1) {   // keep one slab per cache, return the rest
            unlink_slab(c, s);
            env_.put_slab(s);
        }
    }

    int cache_stats(CacheStats* out) const
    {
        for (int i = 0; i < kClasses; ++i) {
            out[i] = CacheStats{caches_[i].name, caches_[i].size, caches_[i].live, caches_[i].slabs};
        }
        return kClasses;
    }
    uint64_t large_live() const { return large_live_; }
    uint64_t large_pages() const { return large_pages_; }
    uint64_t live_objects() const
    {
        uint64_t n = large_live_;
        for (const Cache& c : caches_) {
            n += c.live;
        }
        return n;
    }
    // Returns every completely free slab to the environment (used before leak checks).
    void trim()
    {
        for (Cache& c : caches_) {
            Slab* s = c.slabs_head;
            while (s != nullptr) {
                Slab* next = s->next;
                if (s->in_use == 0) {
                    unlink_slab(c, s);
                    env_.put_slab(s);
                }
                s = next;
            }
        }
    }

private:
    static constexpr uint32_t kSlabMagic = 0x51AB51AB;
    static constexpr uint64_t kLargeMagic = 0x1A26E1A26E1A26E1ull;
    struct Cache;
    struct Slab {                              // lives in the first bytes of the slab
        uint32_t magic;
        uint32_t in_use;
        Cache* cache;
        Slab* next;
        Slab* prev;
        void* free_list;
    };
    struct Cache {
        const char* name;
        size_t size;
        Slab* slabs_head;
        uint64_t live;
        uint64_t slabs;
    };
    struct LargeHeader {                       // in the page in front of a large block
        uint64_t magic;
        size_t pages;
    };

    static Slab* slab_of(void* p)
    {
        return reinterpret_cast<Slab*>(reinterpret_cast<uintptr_t>(p) & ~(kSlabSize - 1));
    }

    static size_t first_offset(size_t size)    // first object: after the header, size-aligned
    {
        size_t off = sizeof(Slab);
        return (off + size - 1) & ~(size - 1);
    }

    bool is_on_free_list(Slab* s, void* p) const
    {
        for (void* q = s->free_list; q != nullptr; q = *static_cast<void**>(q)) {
            if (q == p) {
                return true;
            }
        }
        return false;
    }

    void unlink_slab(Cache& c, Slab* s)
    {
        if (s->prev != nullptr) {
            s->prev->next = s->next;
        } else {
            c.slabs_head = s->next;
        }
        if (s->next != nullptr) {
            s->next->prev = s->prev;
        }
        --c.slabs;
    }

    Slab* new_slab(Cache& c)
    {
        auto* s = static_cast<Slab*>(env_.get_slab());
        if (s == nullptr) {
            return nullptr;
        }
        s->magic = kSlabMagic;
        s->in_use = 0;
        s->cache = &c;
        s->free_list = nullptr;
        auto* base = reinterpret_cast<uint8_t*>(s);
        // Push objects from the top down, so the list hands them out in address order.
        for (size_t off = kSlabSize - c.size; off >= first_offset(c.size); off -= c.size) {
            uint8_t* obj = base + off;
            for (size_t i = sizeof(void*); i < c.size; ++i) {
                obj[i] = kPoison;
            }
            *reinterpret_cast<void**>(obj) = s->free_list;
            s->free_list = obj;
        }
        s->prev = nullptr;
        s->next = c.slabs_head;
        if (s->next != nullptr) {
            s->next->prev = s;
        }
        c.slabs_head = s;
        ++c.slabs;
        return s;
    }

    void* alloc_small(Cache& c)
    {
        Slab* s = c.slabs_head;
        while (s != nullptr && s->free_list == nullptr) {
            s = s->next;
        }
        if (s == nullptr && (s = new_slab(c)) == nullptr) {
            return nullptr;
        }
        void* p = s->free_list;
        s->free_list = *static_cast<void**>(p);
        auto* bytes = static_cast<uint8_t*>(p);
        for (size_t i = sizeof(void*); i < c.size; ++i) {
            if (bytes[i] != kPoison) {         // somebody wrote to a free object
                env_.report("poison check failed (write after free?)", c.name, p, i, bytes[i]);
                break;
            }
        }
        ++s->in_use;
        ++c.live;
        return p;
    }

    void* alloc_large(size_t size)
    {
        size_t pages = (size + kPage - 1) / kPage + 1;   // one extra page for the header
        auto* base = static_cast<uint8_t*>(env_.get_pages(pages));
        if (base == nullptr) {
            return nullptr;
        }
        auto* h = reinterpret_cast<LargeHeader*>(base);
        h->magic = kLargeMagic;
        h->pages = pages;
        ++large_live_;
        large_pages_ += pages;
        return base + kPage;
    }

    void free_large(void* p)
    {
        auto* base = static_cast<uint8_t*>(p) - kPage;
        auto* h = reinterpret_cast<LargeHeader*>(base);
        if (h->magic != kLargeMagic) {
            env_.report("large free with a damaged header", "large", p, 0, 0);
            return;
        }
        h->magic = 0;
        --large_live_;
        large_pages_ -= h->pages;
        env_.put_pages(base, h->pages);
    }

    HeapEnv env_{};
    Cache caches_[kClasses]{};
    uint64_t large_live_ = 0;
    uint64_t large_pages_ = 0;
};
