// kprint.h - F3-18: formatted kernel output to every registered sink (COM1 first).
#pragma once
#include "kformat.h"

void kprint_add_sink(KSink sink, void* ctx);   // later chapters add the framebuffer console
// Optional output lock (F3-23 installs one). After kprint_panic_mode(), output never locks:
// a panic must be able to print even if the code it interrupted held the lock.
void kprint_set_lock(void (*lock)(), void (*unlock)());
void kprint_panic_mode();
void kprint_set_panic_screen(void (*draw)());   // called once, when panic mode starts
void kprintf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void kvprintf(const char* fmt, va_list ap);
void kvprintf_nolock(const char* fmt, va_list ap);   // for callers that hold the lock already
