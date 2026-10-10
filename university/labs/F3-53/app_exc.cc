// app_exc.cc - F3-53 forensic: the program from the bug report. It throws twice: once inside
// itself, once from the plugin libthrower.so, which it loads with dlopen; both exceptions are
// caught in main.
#include <cstdio>
#include <dlfcn.h>
#include <stdexcept>

void loader_report(const char* when);              // the port's loader debug dump (port_ldso.cc)

int main()
{
    loader_report("at start of main");
    try {
        throw std::runtime_error("thrown inside the program");
    } catch (const std::exception& e) {
        std::printf("caught: %s\n", e.what());
    }
    void* plugin = dlopen("./libthrower.so", RTLD_NOW);
    if (plugin == nullptr) {
        std::printf("dlopen failed: %s\n", dlerror());
        return 1;
    }
    // the mangled name of void throw_from_library(int)
    auto fn = reinterpret_cast<void (*)(int)>(dlsym(plugin, "_Z18throw_from_libraryi"));
    std::printf("plugin loaded; throw_from_library at %s\n", fn != nullptr ? "a valid address" : "NULL");
    loader_report("after dlopen");
    std::fflush(stdout);
    try {
        fn(7);
    } catch (const std::exception& e) {
        std::printf("caught: %s\n", e.what());
    }
    std::printf("end of main\n");
    return 0;
}
