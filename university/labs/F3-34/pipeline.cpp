// pipeline.cpp - F3-34: three processes connected by two POSIX pipes move 1 GiB; the checksums
// computed by the first and the last process are compared (curriculum B17, first acceptance test,
// run on the host's kernel as the reference behaviour your kernel must reproduce).
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

struct Result { std::uint64_t sent_bytes, sent_sum, got_bytes, got_sum; };

static std::uint64_t next_word(std::uint64_t& s) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }

static bool write_all(int fd, const void* p, std::size_t n)     // write() may write less than asked
{
    const char* c = static_cast<const char*>(p);
    while (n > 0) {
        ssize_t k = write(fd, c, n);
        if (k < 0) return false;
        c += k;
        n -= static_cast<std::size_t>(k);
    }
    return true;
}

int main()
{
    const std::uint64_t total = 1ull << 30;                     // 1 GiB
    // The result record lives in memory shared by all four processes.
    void* mem = mmap(nullptr, sizeof(Result), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { std::perror("mmap"); return 1; }
    auto* r = static_cast<Result*>(mem);
    std::memset(r, 0, sizeof *r);
    int a[2], b[2];
    if (pipe(a) != 0 || pipe(b) != 0) { std::perror("pipe"); return 1; }

    if (fork() == 0) {                                          // 1: generator -> a
        close(a[0]); close(b[0]); close(b[1]);
        std::uint64_t s = 304, buf[8192];
        for (std::uint64_t off = 0; off < total; off += sizeof buf) {
            for (auto& w : buf) { w = next_word(s); r->sent_sum += w; }
            if (!write_all(a[1], buf, sizeof buf)) _exit(1);
            r->sent_bytes += sizeof buf;
        }
        _exit(0);                                               // closing a[1] tells the reader: end of file
    }
    if (fork() == 0) {                                          // 2: a -> b, like "cat"
        close(a[1]); close(b[0]);
        char buf[65536];
        ssize_t n;
        while ((n = read(a[0], buf, sizeof buf)) > 0)
            if (!write_all(b[1], buf, static_cast<std::size_t>(n))) _exit(1);
        _exit(n < 0 ? 1 : 0);
    }
    if (fork() == 0) {                                          // 3: b -> checksum
        close(a[0]); close(a[1]); close(b[1]);
        std::uint8_t buf[65536 + 8];
        std::size_t have = 0;                                   // bytes of a partial word kept in buf
        ssize_t n;
        while ((n = read(b[0], buf + have, 65536)) > 0) {
            std::size_t len = have + static_cast<std::size_t>(n), i = 0;
            for (; i + 8 <= len; i += 8) { std::uint64_t w; std::memcpy(&w, buf + i, 8); r->got_sum += w; }
            have = len - i;
            std::memmove(buf, buf + i, have);
            r->got_bytes += static_cast<std::uint64_t>(n);
        }
        _exit(n < 0 ? 1 : 0);
    }
    close(a[0]); close(a[1]); close(b[0]); close(b[1]);         // the parent keeps no pipe ends open
    int failures = 0, st = 0;
    for (int i = 0; i < 3; ++i) { wait(&st); if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) ++failures; }
    std::printf("sent %llu bytes, sum 0x%016llx\n", static_cast<unsigned long long>(r->sent_bytes),
                static_cast<unsigned long long>(r->sent_sum));
    std::printf("got  %llu bytes, sum 0x%016llx\n", static_cast<unsigned long long>(r->got_bytes),
                static_cast<unsigned long long>(r->got_sum));
    bool ok = failures == 0 && r->got_bytes == total && r->sent_sum == r->got_sum;
    std::printf("B17 pipeline test: %s\n", ok ? "PASS (1 GiB, checksums equal)" : "FAIL");
    munmap(mem, sizeof(Result));
    return ok ? 0 : 1;
}
