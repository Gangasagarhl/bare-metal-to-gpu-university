// new.cc - F3-22: every replaceable form of operator new and delete, on top of kmalloc.
// The kernel is built without exceptions: the throwing forms panic instead of throwing
// std::bad_alloc; the nothrow forms return nullptr. (The forms are listed in the C++
// standard's [new.delete] section; pending verification, F3-22 D2.)
#include <cstddef>
#include <new>
#include "kheap.h"
#include "panic.h"

// The object std::nothrow normally comes from the C++ runtime library (libsupc++), which a
// freestanding kernel does not link, so the kernel's runtime defines it.
namespace std {
const nothrow_t nothrow{};
}

namespace {
void* must(void* p, size_t size)
{
    if (p == nullptr) {
        PANIC("operator new: out of memory for %lu bytes", size);
    }
    return p;
}
} // namespace

void* operator new(size_t size) { return must(kmalloc(size), size); }
void* operator new[](size_t size) { return must(kmalloc(size), size); }
void* operator new(size_t size, std::align_val_t a) { return must(kmalloc(size, static_cast<size_t>(a)), size); }
void* operator new[](size_t size, std::align_val_t a) { return must(kmalloc(size, static_cast<size_t>(a)), size); }
void* operator new(size_t size, const std::nothrow_t&) noexcept { return kmalloc(size); }
void* operator new[](size_t size, const std::nothrow_t&) noexcept { return kmalloc(size); }
void* operator new(size_t size, std::align_val_t a, const std::nothrow_t&) noexcept
{
    return kmalloc(size, static_cast<size_t>(a));
}
void* operator new[](size_t size, std::align_val_t a, const std::nothrow_t&) noexcept
{
    return kmalloc(size, static_cast<size_t>(a));
}

void operator delete(void* p) noexcept { kfree(p); }
void operator delete[](void* p) noexcept { kfree(p); }
void operator delete(void* p, size_t) noexcept { kfree(p); }
void operator delete[](void* p, size_t) noexcept { kfree(p); }
void operator delete(void* p, std::align_val_t) noexcept { kfree(p); }
void operator delete[](void* p, std::align_val_t) noexcept { kfree(p); }
void operator delete(void* p, size_t, std::align_val_t) noexcept { kfree(p); }
void operator delete[](void* p, size_t, std::align_val_t) noexcept { kfree(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { kfree(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { kfree(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { kfree(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { kfree(p); }
