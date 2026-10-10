// kmain.cpp: freestanding C++ in a tiny kernel: global constructors, a virtual call,
// operator new and a function-local static, all without the C++ library.
#include "console.h"

extern "C" int g_atexit_registrations;

// A global object whose constructor has a side effect, so it cannot be done at compile
// time: the compiler emits an initialiser function and puts its address in .init_array.
struct Log {
    int lines;
    Log() : lines(0)
    {
        print("[init] Log constructor ran\n");
        lines = 1;
    }
    ~Log() {}   // non-trivial destructor: registered with __cxa_atexit
};
Log g_log;

struct Device {
    virtual const char* name() const = 0;   // pure virtual: needs __cxa_pure_virtual
    virtual ~Device() = default;
};
struct Uart : Device {
    const char* name() const override { return "uart0"; }
};

int nextTicket()
{
    static int counter = 100;   // constant initial value: no guard needed
    return ++counter;
}

// Boundaries of .init_array, exported by link.ld.
using InitFn = void (*)();
extern "C" InitFn __init_array_start[], __init_array_end[];

void runGlobalConstructors()
{
    for (InitFn* f = __init_array_start; f != __init_array_end; ++f) {
        (*f)();
    }
}

extern "C" void kmain()
{
    print("F2-48 kernel: freestanding C++\n");
    print("init_array entries: ");
    printDec(static_cast<uint32_t>(__init_array_end - __init_array_start));
    print("\n");
    runGlobalConstructors();
    print("g_log.lines = ");
    printDec(static_cast<uint32_t>(g_log.lines));
    print(g_log.lines == 1 ? " (constructed)\n" : " (NOT constructed)\n");
    print("__cxa_atexit registrations: ");
    printDec(static_cast<uint32_t>(g_atexit_registrations));
    print("\n");
    Device* d = new Uart;   // operator new from runtime.cpp
    print("virtual call: ");
    print(d->name());
    print("\n");
    delete d;
    print("tickets: ");
    printDec(static_cast<uint32_t>(nextTicket()));
    print(" ");
    printDec(static_cast<uint32_t>(nextTicket()));
    print("\n");
    qemuExit(g_log.lines == 1 ? 0x10 : 0x11);   // 33 = success, 35 = global not constructed
}
