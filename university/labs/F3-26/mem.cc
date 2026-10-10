// mem.cc - boot information, physical frames, kernel heap and 4 KiB kernel mappings.
// (The full treatment of these is OS302: F3-20 to F3-22. This is the small version
// the OS303 labs need.)
#include "cpu.h"
#include "sync.h"

extern "C" uint8_t __kernel_end[];
extern "C" uint64_t boot_pdpt[512];

namespace k {

// ------------------------------------------------------------ Multiboot information
// Field offsets of the Multiboot (version 1) information structure, as our boot code
// reads them; see the chapter's unverified box for the document to check.
namespace {
char cmdline[256];
BootModule modules[16];
int nmodules = 0;
uint64_t mem_top = 0;

template <typename T> T rd(uint64_t addr) { return *reinterpret_cast<const T*>(addr); }
}  // namespace

const char* boot_cmdline() { return cmdline; }
int boot_module_count() { return nmodules; }
const BootModule* boot_module(int i) { return &modules[i]; }

const BootModule* boot_module_find(const char* name)
{
    for (int i = 0; i < nmodules; ++i) {
        if (str_eq(modules[i].name, name)) {
            return &modules[i];
        }
    }
    return nullptr;
}

const char* boot_arg(const char* key)   // "test=threads cpus=4": boot_arg("test") -> "threads"
{
    static char value[64];
    size_t klen = str_len(key);
    for (const char* p = cmdline; *p != '\0'; ++p) {
        if ((p == cmdline || p[-1] == ' ') && memcmp(p, key, klen) == 0 && p[klen] == '=') {
            const char* v = p + klen + 1;
            size_t n = 0;
            while (v[n] != '\0' && v[n] != ' ' && n + 1 < sizeof(value)) {
                value[n] = v[n];
                ++n;
            }
            value[n] = '\0';
            return value;
        }
    }
    return nullptr;
}

// ------------------------------------------------------------ physical frames
namespace {
constexpr uint64_t kMaxFrames = (1ull << 30) / kPage;   // this kernel uses at most 1 GiB
uint64_t bitmap[kMaxFrames / 64];                        // 1 = used
uint64_t nfree = 0, ntotal = 0, hint = 0;
Spinlock frame_lock("frames");

void mark(uint64_t pa, bool used)
{
    uint64_t f = pa / kPage;
    if (f >= kMaxFrames) {
        return;
    }
    bool was = (bitmap[f / 64] >> (f % 64)) & 1;
    if (used && !was) {
        bitmap[f / 64] |= 1ull << (f % 64);
        --nfree;
    } else if (!used && was) {
        bitmap[f / 64] &= ~(1ull << (f % 64));
        ++nfree;
    }
}
}  // namespace

void mem_init(uint64_t mbi)
{
    uint32_t flags = rd<uint32_t>(mbi);
    if (flags & (1u << 2)) {   // command line present
        str_copy(cmdline, reinterpret_cast<const char*>(uint64_t(rd<uint32_t>(mbi + 16))),
                 sizeof(cmdline));
    }
    for (auto& w : bitmap) {
        w = ~0ull;
    }
    nfree = 0;
    if (flags & (1u << 6)) {   // memory map present: entries of {size, base, length, type}
        uint64_t p = rd<uint32_t>(mbi + 48), end = p + rd<uint32_t>(mbi + 44);
        kprintf("memory map (from the boot loader):\n");
        while (p < end) {
            uint32_t size = rd<uint32_t>(p);
            uint64_t base = rd<uint64_t>(p + 4), len = rd<uint64_t>(p + 12);
            uint32_t type = rd<uint32_t>(p + 20);
            kprintf("  %016lx-%016lx type %u%s\n", base, base + len - 1, type,
                    type == 1 ? " (usable RAM)" : "");
            if (type == 1) {
                for (uint64_t a = (base + kPage - 1) & ~(kPage - 1); a + kPage <= base + len; a += kPage) {
                    if (a >= 0x100000) {   // keep the first MiB (BIOS data, SMP trampoline)
                        mark(a, false);
                        mem_top = a + kPage > mem_top ? a + kPage : mem_top;
                    }
                }
            }
            p += size + 4;
        }
    }
    for (uint64_t a = 0x100000; a < reinterpret_cast<uint64_t>(__kernel_end); a += kPage) {
        mark(a, true);   // the kernel image
    }
    if (flags & (1u << 3)) {   // boot modules: {start, end, string, reserved}
        uint32_t count = rd<uint32_t>(mbi + 20);
        uint64_t m = rd<uint32_t>(mbi + 24);
        for (uint32_t i = 0; i < count && nmodules < 16; ++i, m += 16) {
            uint64_t s = rd<uint32_t>(m), e = rd<uint32_t>(m + 4);
            const char* str = reinterpret_cast<const char*>(uint64_t(rd<uint32_t>(m + 8)));
            const char* base = str;   // keep only the file name after the last '/'
            for (const char* q = str; *q != '\0' && *q != ' '; ++q) {
                if (*q == '/') {
                    base = q + 1;
                }
            }
            BootModule& bm = modules[nmodules++];
            size_t n = 0;
            while (base[n] != '\0' && base[n] != ' ' && n + 1 < sizeof(bm.name)) {
                bm.name[n] = base[n];
                ++n;
            }
            bm.name[n] = '\0';
            bm.data = reinterpret_cast<const uint8_t*>(s);
            bm.size = e - s;
            for (uint64_t a = s & ~(kPage - 1); a < e; a += kPage) {
                mark(a, true);
            }
        }
    }
    ntotal = nfree;
    kprintf("frames: %lu free of 4 KiB (%lu KiB), kernel ends at %p, %d boot modules\n", nfree,
            nfree * 4, static_cast<void*>(__kernel_end), nmodules);
}

uint64_t frame_alloc()
{
    uint64_t f = frame_lock.lock_irqsave();
    uint64_t found = 0;
    for (uint64_t i = 0; i < kMaxFrames / 64 && found == 0; ++i) {
        uint64_t w = (hint + i) % (kMaxFrames / 64);
        if (bitmap[w] != ~0ull) {
            int bit = __builtin_ctzll(~bitmap[w]);
            found = (w * 64 + bit) * kPage;
            mark(found, true);
            hint = w;
        }
    }
    frame_lock.unlock_irqrestore(f);
    if (found == 0) {
        panic("out of physical memory");
    }
    memset(reinterpret_cast<void*>(found), 0, kPage);   // identity-mapped: pa == va
    return found;
}

void frame_free(uint64_t pa)
{
    uint64_t f = frame_lock.lock_irqsave();
    KASSERT(((bitmap[pa / kPage / 64] >> (pa / kPage % 64)) & 1) != 0);   // double free?
    mark(pa, false);
    frame_lock.unlock_irqrestore(f);
}

uint64_t frames_free() { return nfree; }
uint64_t frames_total() { return ntotal; }

// ------------------------------------------------------------ kernel heap (slabs)
namespace {
struct Slab {
    Slab* next;
    Slab* prev;
    void* free;        // free objects of this slab, linked through their first word
    uint32_t inuse, capacity, cls;
};
constexpr int kClasses = 8;   // 16, 32, ..., 2048 bytes
constexpr size_t kHeader = 64;
Slab* partial[kClasses];      // slabs with at least one free object
uint64_t live_objects = 0;
Spinlock heap_lock("heap");

void unlink(Slab* s, int c)
{
    if (s->prev) {
        s->prev->next = s->next;
    } else {
        partial[c] = s->next;
    }
    if (s->next) {
        s->next->prev = s->prev;
    }
    s->next = s->prev = nullptr;
}
}  // namespace

void* kmalloc(size_t n)
{
    int c = 0;
    while ((size_t(16) << c) < n) {
        ++c;
    }
    if (c >= kClasses) {
        panic("kmalloc(%lu): too large for this small heap", n);
    }
    uint64_t f = heap_lock.lock_irqsave();
    Slab* s = partial[c];
    if (s == nullptr) {
        heap_lock.unlock_irqrestore(f);
        auto* fresh = reinterpret_cast<Slab*>(frame_alloc());
        size_t size = size_t(16) << c;
        fresh->cls = c;
        fresh->capacity = uint32_t((kPage - kHeader) / size);
        for (uint32_t i = 0; i < fresh->capacity; ++i) {   // build the free list
            auto* obj = reinterpret_cast<uint8_t*>(fresh) + kHeader + i * size;
            *reinterpret_cast<void**>(obj) = fresh->free;
            fresh->free = obj;
        }
        f = heap_lock.lock_irqsave();
        fresh->next = partial[c];
        if (partial[c]) {
            partial[c]->prev = fresh;
        }
        partial[c] = s = fresh;
    }
    void* obj = s->free;
    s->free = *static_cast<void**>(obj);
    if (++s->inuse == s->capacity) {
        unlink(s, c);
    }
    ++live_objects;
    heap_lock.unlock_irqrestore(f);
    memset(obj, 0, size_t(16) << c);
    return obj;
}

void kfree(void* p)
{
    if (p == nullptr) {
        return;
    }
    auto* s = reinterpret_cast<Slab*>(reinterpret_cast<uint64_t>(p) & ~(kPage - 1));
    uint64_t f = heap_lock.lock_irqsave();
    bool was_full = s->inuse == s->capacity;
    *static_cast<void**>(p) = s->free;
    s->free = p;
    --s->inuse;
    --live_objects;
    if (was_full) {   // back on the partial list
        s->next = partial[s->cls];
        s->prev = nullptr;
        if (partial[s->cls]) {
            partial[s->cls]->prev = s;
        }
        partial[s->cls] = s;
    }
    bool empty = s->inuse == 0;
    if (empty) {
        unlink(s, int(s->cls));
    }
    heap_lock.unlock_irqrestore(f);
    if (empty) {
        frame_free(reinterpret_cast<uint64_t>(s));   // empty slabs go back to the frames
    }
}

uint64_t heap_live_objects() { return live_objects; }

// ------------------------------------------------------------ 4 KiB kernel mappings
// Kernel mappings above 4 GiB live under PML4 entry 0 -> boot_pdpt, which every
// address space shares, so they appear in all of them at once.
namespace {
Spinlock kmap_lock("kmap");
constexpr uint64_t kPresent = 1, kWritable = 2, kAddrMask = 0x000FFFFFFFFFF000ull;

uint64_t* pte_for(uint64_t va, bool create)
{
    uint64_t* pdpt = boot_pdpt;
    uint64_t i3 = (va >> 30) & 511, i2 = (va >> 21) & 511, i1 = (va >> 12) & 511;
    KASSERT((va >> 39) == 0 && i3 >= 4);   // only the area above the 4 GiB identity map
    if (!(pdpt[i3] & kPresent)) {
        if (!create) {
            return nullptr;
        }
        pdpt[i3] = frame_alloc() | kPresent | kWritable;
    }
    auto* pd = reinterpret_cast<uint64_t*>(pdpt[i3] & kAddrMask);
    if (!(pd[i2] & kPresent)) {
        if (!create) {
            return nullptr;
        }
        pd[i2] = frame_alloc() | kPresent | kWritable;
    }
    auto* pt = reinterpret_cast<uint64_t*>(pd[i2] & kAddrMask);
    return &pt[i1];
}
}  // namespace

void kmap_page(uint64_t va, uint64_t pa)
{
    uint64_t f = kmap_lock.lock_irqsave();
    uint64_t* pte = pte_for(va, true);
    KASSERT(!(*pte & kPresent));
    *pte = pa | kPresent | kWritable;
    kmap_lock.unlock_irqrestore(f);
}

uint64_t kunmap_page(uint64_t va)
{
    uint64_t f = kmap_lock.lock_irqsave();
    uint64_t* pte = pte_for(va, false);
    uint64_t pa = (pte && (*pte & kPresent)) ? (*pte & kAddrMask) : 0;
    if (pte) {
        *pte = 0;
    }
    kmap_lock.unlock_irqrestore(f);
    return pa;
}

bool kpage_mapped(uint64_t va)
{
    uint64_t f = kmap_lock.lock_irqsave();
    uint64_t* pte = pte_for(va, false);
    bool r = pte && (*pte & kPresent);
    kmap_lock.unlock_irqrestore(f);
    return r;
}

}  // namespace k
