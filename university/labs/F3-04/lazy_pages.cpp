// lazy_pages.cpp - the kernel gives a process memory lazily: a page becomes real only when
// it is first touched, and each first touch is a (minor) page fault handled by the kernel.
#include <cstdio>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

static long minorFaults()
{
    rusage u{};
    getrusage(RUSAGE_SELF, &u);
    return u.ru_minflt;
}

int main()
{
    const long page = sysconf(_SC_PAGESIZE);
    const long pages = 4096;
    const std::size_t bytes = static_cast<std::size_t>(page * pages);
    void* mem = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        std::perror("mmap");
        return 1;
    }
    char* p = static_cast<char*>(mem);
    const long beforeTouch = minorFaults();
    for (long i = 0; i < pages; ++i) {
        p[i * page] = 1;  // first write to each page
    }
    const long firstPass = minorFaults() - beforeTouch;
    const long beforeSecond = minorFaults();
    for (long i = 0; i < pages; ++i) {
        p[i * page] = 2;  // the pages are real now
    }
    const long secondPass = minorFaults() - beforeSecond;
    std::printf("page size %ld bytes, region of %ld pages mapped\n", page, pages);
    std::printf("minor page faults while touching each page the first time:  %ld\n", firstPass);
    std::printf("minor page faults while touching each page the second time: %ld\n", secondPass);
    munmap(mem, bytes);
    return 0;
}
