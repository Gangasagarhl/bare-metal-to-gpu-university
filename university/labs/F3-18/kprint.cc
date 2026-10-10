// kprint.cc - F3-18: fan-out of formatted characters to up to four sinks.
#include "kprint.h"
#include "serial.h"

namespace {
struct Sink {
    KSink fn;
    void* ctx;
};
Sink g_sinks[4];
int g_nsinks = 0;
void (*g_lock)() = nullptr;
void (*g_unlock)() = nullptr;
bool g_panic_mode = false;
void (*g_panic_screen)() = nullptr;

void to_all(char c, void*)
{
    if (g_nsinks == 0) {
        serial::put(c);          // before anyone registered a sink: COM1 directly
        return;
    }
    for (int i = 0; i < g_nsinks; ++i) {
        g_sinks[i].fn(c, g_sinks[i].ctx);
    }
}
} // namespace

void kprint_add_sink(KSink sink, void* ctx)
{
    if (g_nsinks < 4) {
        g_sinks[g_nsinks++] = Sink{sink, ctx};
    }
}

void kprint_set_lock(void (*lock)(), void (*unlock)())
{
    g_lock = lock;
    g_unlock = unlock;
}

void kprint_set_panic_screen(void (*draw)())
{
    g_panic_screen = draw;
}

void kprint_panic_mode()
{
#if !defined(PANIC_KEEPS_LOCK)     // the F3-23 forensic kernel is built with this defined
    if (!g_panic_mode) {
        g_panic_mode = true;
        if (g_panic_screen != nullptr) {
            g_panic_screen();
        }
    }
#endif
}

void kvprintf(const char* fmt, va_list ap)
{
    bool locking = g_lock != nullptr && !g_panic_mode;
    if (locking) {
        g_lock();
    }
    kvformat(to_all, nullptr, fmt, ap);
    if (locking) {
        g_unlock();
    }
}

void kvprintf_nolock(const char* fmt, va_list ap)
{
    kvformat(to_all, nullptr, fmt, ap);
}

void kprintf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
}
