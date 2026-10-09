// port_ldso_fixed.cc - F3-53 forensic answer key: port_ldso.cc with the fix.
// Stands in for the port's dynamic linker. The host's dynamic
// linker still does the loading; this file replaces the one lookup service the C++ unwinder
// uses to ask "which loaded object contains this code address, and where is its unwind data?".
// On this host's C library that service is _dl_find_object (declared in <dlfcn.h>); the
// curriculum (8.1) names dl_iterate_phdr for the same role. The port's logic is reproduced below.
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <link.h>
#include <set>

namespace {

using FindFn = int (*)(void*, dl_find_object*);

FindFn host_find()
{
    static FindFn f = reinterpret_cast<FindFn>(dlsym(RTLD_NEXT, "_dl_find_object"));
    return f;
}

std::set<ElfW(Addr)>& known_objects()               // load addresses the port's table knows
{
    static std::set<ElfW(Addr)> s;
    return s;
}

int record(dl_phdr_info* info, size_t, void*)
{
    known_objects().insert(info->dlpi_addr);
    return 0;
}

// The port fills its object table once, when the program starts.
__attribute__((constructor)) void build_table()
{
    dl_iterate_phdr(record, nullptr);
}

const char* base_name(const char* p)
{
    const char* s = std::strrchr(p, '/');
    return p[0] == '\0' ? "(the program)" : (s != nullptr ? s + 1 : p);
}

int dump(dl_phdr_info* info, size_t, void*)
{
    bool known = known_objects().count(info->dlpi_addr) != 0;
    std::printf("  %-24s in the port's table: %s\n", base_name(info->dlpi_name), known ? "yes" : "no");
    return 0;
}

} // namespace

// The fix: the table follows dlopen and dlclose. (A real dynamic linker updates its list inside
// dlopen/dlclose; here the table is rebuilt from the loaded-object list on a miss.)
extern "C" int _dl_find_object(void* address, dl_find_object* result)
{
    dl_find_object tmp;
    if (host_find()(address, &tmp) != 0) return -1;
    if (known_objects().count(tmp.dlfo_link_map->l_addr) == 0) {
        known_objects().clear();
        dl_iterate_phdr(record, nullptr);
        if (known_objects().count(tmp.dlfo_link_map->l_addr) == 0) return -1;
    }
    *result = tmp;
    return 0;
}

void loader_report(const char* when)
{
    std::printf("loader debug dump (%s): objects in memory, and whether the lookup service knows them\n", when);
    dl_iterate_phdr(dump, nullptr);
}
