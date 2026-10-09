// inside.cc - what a process can see of its world. Run once on the host and once
// inside a runc container (run.sh builds it static: the container has no libraries).
#include <dirent.h>
#include <unistd.h>

#include <cctype>
#include <cstdio>
#include <fstream>
#include <set>
#include <string>

std::set<std::string> listDir(const char* path)
{
    std::set<std::string> names;
    if (DIR* d = opendir(path)) {
        while (const dirent* e = readdir(d)) {
            const std::string n = e->d_name;
            if (n != "." && n != "..") {
                names.insert(n);
            }
        }
        closedir(d);
    }
    return names;
}

int main()
{
    char host[65] = {};
    gethostname(host, sizeof(host) - 1);
    std::printf("pid %d, parent pid %d, uid %d, host name %s\n", static_cast<int>(getpid()),
                static_cast<int>(getppid()), static_cast<int>(getuid()), host);

    int processes = 0;
    for (const std::string& n : listDir("/proc")) {
        processes += std::isdigit(static_cast<unsigned char>(n[0])) ? 1 : 0;
    }
    std::printf("processes visible in /proc: %d\n", processes);

    const std::set<std::string> root = listDir("/");
    std::printf("entries in / : %zu (", root.size());
    int shown = 0;
    for (const std::string& n : root) {
        if (shown++ < 8) {
            std::printf("%s%s", shown > 1 ? " " : "", n.c_str());
        }
    }
    std::printf("%s)\n", root.size() > 8 ? " ..." : "");

    std::ifstream limit("/sys/fs/cgroup/memory/memory.limit_in_bytes");
    std::string value;
    if (limit >> value) {
        std::printf("memory.limit_in_bytes seen by this process: %s\n", value.c_str());
    } else {
        std::printf("memory.limit_in_bytes: not readable here\n");
    }
    return 0;
}
