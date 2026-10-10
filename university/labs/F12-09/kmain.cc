// kmain.cc - the CI test kernel. It runs the tests named on its command line, prints one
// structured line per test on the serial port (COM1), and ends the QEMU run through QEMU's
// isa-debug-exit device: 0x10 (QEMU exit status 33) = every test passed,
// 0x11 (exit status 35) = a test failed or the kernel panicked.
#include <stdint.h>

#include "bitmap.hpp"

namespace {

void outb(uint16_t port, uint8_t v) { __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }

uint8_t inb(uint16_t port)
{
    uint8_t v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

void putc(char c)
{
    while ((inb(0x3f8 + 5) & 0x20) == 0) {  // wait until the transmitter can take a byte
    }
    outb(0x3f8, static_cast<uint8_t>(c));
}

void print(const char* s)
{
    for (; *s != '\0'; ++s) {
        putc(*s);
    }
}

void dec(uint32_t v)
{
    char buf[11];
    int i = 10;
    buf[i] = '\0';
    do {
        buf[--i] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v != 0);
    print(&buf[i]);
}

[[noreturn]] void qemu_exit(uint8_t code)
{
    outb(0xf4, code);  // isa-debug-exit at port 0xf4: QEMU exits with status (code << 1) | 1
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

[[noreturn]] void panic(const char* what, int line)
{
    print("PANIC: assertion failed: ");
    print(what);
    print(" at kmain.cc:");
    dec(static_cast<uint32_t>(line));
    print("\n");
    qemu_exit(0x11);
}

#define KASSERT(x)                    \
    do {                              \
        if (!(x)) {                   \
            panic(#x, __LINE__);      \
        }                             \
    } while (0)

bool contains(const char* text, const char* word)
{
    for (; *text != '\0'; ++text) {
        const char* a = text;
        const char* b = word;
        while (*b != '\0' && *a == *b) {
            ++a;
            ++b;
        }
        if (*b == '\0') {
            return true;
        }
    }
    return false;
}

bool test_bitmap()  // curriculum B3: allocate everything, free everything, same free count
{
    FrameBitmap b(128);
    const uint32_t start = b.free_count();
    uint32_t seen[4] = {0, 0, 0, 0};
    for (int i = 0; i < 128; ++i) {
        const int32_t f = b.alloc();
        if (f < 0 || (seen[f / 32] & (1u << (f % 32))) != 0) {
            return false;  // out of frames too early, or a frame handed out twice
        }
        seen[f / 32] |= 1u << (f % 32);
    }
    const bool empty_ok = b.alloc() == -1 && b.free_count() == 0;
    for (int32_t f = 0; f < 128; ++f) {
        b.release(f);
    }
    print("KTEST bitmap: free count ");
    dec(start);
    print(" -> 0 -> ");
    dec(b.free_count());
    print("\n");
    return empty_ok && b.free_count() == start;
}

bool test_selfcheck(uint32_t magic, const char* cmdline)
{
    print("KTEST selfcheck: command line '");
    print(cmdline);
    print("'\n");
    return magic == 0x2BADB002;
}

}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t info)
{
    const char* cmdline = "";
    if (magic == 0x2BADB002) {  // a Multiboot loader started us; info points at its structure
        const auto* mbi = reinterpret_cast<const uint32_t*>(static_cast<uintptr_t>(info));
        if ((mbi[0] & (1u << 2)) != 0) {  // flags bit 2: the cmdline field (offset 16) is valid
            cmdline = reinterpret_cast<const char*>(static_cast<uintptr_t>(mbi[4]));
        }
    }
    print("KTEST start\n");
    const bool all = contains(cmdline, "test=all");
    uint32_t passed = 0;
    uint32_t failed = 0;
    auto report = [&](const char* name, bool ok) {
        print("KTEST ");
        print(name);
        print(ok ? " ok\n" : " FAILED\n");
        (ok ? passed : failed) += 1;
    };
    if (all || contains(cmdline, "bitmap")) {
        report("bitmap", test_bitmap());
    }
    if (all || contains(cmdline, "selfcheck")) {
        report("selfcheck", test_selfcheck(magic, cmdline));
    }
    if (contains(cmdline, "panic")) {  // harness self-test: a deliberately wrong assertion
        FrameBitmap b(128);
        b.alloc();
        b.alloc();
        KASSERT(b.free_count() == 127);
    }
    if (contains(cmdline, "hang")) {  // harness self-test: a kernel that never finishes
        print("KTEST hang: halting with interrupts off\n");
        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }
    if (passed + failed == 0) {
        print("KTEST no test selected\n");  // "nothing ran" is a failure, not a pass
        failed = 1;
    }
    print("KTEST summary ");
    dec(passed);
    print(" passed ");
    dec(failed);
    print(" failed\n");
    qemu_exit(failed == 0 ? 0x10 : 0x11);
}
