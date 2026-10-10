// log.cc - F3-23: levels, prefixes, colours, the output lock and the panic screen.
#include "log.h"
#include "arch.h"
#include "fb.h"
#include "fbconsole.h"
#include "kprint.h"
#include "serial.h"

namespace {

FbConsole g_console;
bool g_have_console = false;
Level g_min = Level::Debug;
uint64_t (*g_now_ns)() = nullptr;
uint64_t g_seq = 0;

volatile int g_lock_word = 0;
uint64_t g_saved_flags = 0;

constexpr uint32_t kColor[4] = {0x00808890, 0x00d0d0d0, 0x00f0d040, 0x00ff6060};   // by level
const char* const kName[4] = {"DEBUG", " INFO", " WARN", "ERROR"};

void serial_sink(char c, void*)
{
    serial::put(c);
}

void console_sink(char c, void*)
{
    g_console.put(c);
}

void panic_screen()
{
    if (!g_have_console) {
        return;
    }
    g_console.set_colors(0x00ffffff, 0x00a01010);
    g_console.clear(0x00a01010);
    const char* title = "*** KERNEL PANIC: THE SYSTEM IS HALTED ***\n\n";
    for (const char* p = title; *p != '\0'; ++p) {
        g_console.put(*p);
    }
}

} // namespace

// The lock masks interrupts on this CPU first (an interrupt handler that logs must not spin
// on a lock held by the code it interrupted), then spins with an atomic exchange.
void log_lock_acquire()
{
    uint64_t flags = arch::save_flags_cli();
    while (__atomic_exchange_n(&g_lock_word, 1, __ATOMIC_ACQUIRE) != 0) {
        arch::pause();
    }
    g_saved_flags = flags;
}

void log_lock_release()
{
    uint64_t flags = g_saved_flags;
    __atomic_store_n(&g_lock_word, 0, __ATOMIC_RELEASE);
    arch::restore_flags(flags);             // interrupts back on only if they were on before
}

void log_init(bool with_framebuffer)
{
    kprint_add_sink(serial_sink, nullptr);
    fb::Info info{};
    if (with_framebuffer && fb::init(640, 480, info)) {
        g_console.init(info.pixels, info.width, info.height, info.pitch_pixels);
        kprint_add_sink(console_sink, nullptr);
        g_have_console = true;
    }
    kprint_set_lock(log_lock_acquire, log_lock_release);
    kprint_set_panic_screen(panic_screen);
    kprintf("console: started (%s)\n", g_have_console ? "serial and framebuffer" : "serial only");
}

void log_set_min_level(Level level)
{
    g_min = level;
}

void log_set_clock(uint64_t (*now_ns)())
{
    g_now_ns = now_ns;
}

static void print_nolock(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    kvprintf_nolock(fmt, ap);
    va_end(ap);
}

void klog(Level level, const char* fmt, ...)
{
    if (level < g_min) {
        return;
    }
    int l = static_cast<int>(level);
    log_lock_acquire();
    if (g_have_console) {
        g_console.set_colors(kColor[l], FbConsole::kBackground);
    }
    if (g_now_ns != nullptr) {
        uint64_t ns = g_now_ns();
        print_nolock("[%5lu.%06lu] [%s] ", ns / 1000000000, (ns / 1000) % 1000000, kName[l]);
    } else {
        print_nolock("[#%04lu] [%s] ", ++g_seq, kName[l]);
    }
    va_list ap;
    va_start(ap, fmt);
    kvprintf_nolock(fmt, ap);
    va_end(ap);
    print_nolock("\n");
    if (g_have_console) {
        g_console.set_colors(kColor[1], FbConsole::kBackground);
    }
    log_lock_release();
}
