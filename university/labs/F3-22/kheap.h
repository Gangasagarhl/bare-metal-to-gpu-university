// kheap.h - F3-22: the kernel heap: kmalloc/kfree and the statistics the tests read.
#pragma once
#include <cstddef>
#include <cstdint>
#include "heap.h"

void kheap_init();
void* kmalloc(size_t size, size_t align = 16);
void kfree(void* p);
Heap& kheap();
void kheap_report(const char* tag);     // live objects per cache, large blocks, PMM frames
