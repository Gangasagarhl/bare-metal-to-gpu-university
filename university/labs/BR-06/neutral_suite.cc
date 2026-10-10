// neutral_suite.cc - BR-06: the arch-neutral test suite, user-mode edition.
// One source, built unchanged for x86-64, AArch64 and RISC-V (run.sh) and run on each
// (natively, or under QEMU user-mode emulation). The build passes one fact, ARCH_NAME,
// so that the report says where it ran; no test ever branches on it.
// Part 1 prints facts that differ between the machines; part 2 tests code written so
// that those differences do not matter.
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include <thread>
#include <unistd.h>

#ifndef ARCH_NAME
#define ARCH_NAME "unknown"
#endif

namespace {

int g_pass = 0;
int g_fail = 0;

void check(bool ok, const char* what)
{
    std::printf("  %s  %s\n", ok ? "ok  " : "FAIL", what);
    (ok ? g_pass : g_fail) += 1;
}

// Explicit byte order: the only way external data is read (curriculum 10.2).
uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]); }
uint16_t be16(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }

// Page arithmetic that takes the page size as a parameter (from the arch layer).
uint64_t pages_for(uint64_t bytes, uint64_t page) { return (bytes + page - 1) / page; }
uint64_t align_down(uint64_t a, uint64_t page) { return a & ~(page - 1); }

// Byte classification that does not depend on whether plain char is signed.
bool is_high_byte(char c) { return static_cast<unsigned char>(c) >= 0x80; }

// Message passing with release/acquire: the pattern of every descriptor ring.
bool message_passing_rounds(int rounds)
{
    std::atomic<int> turn{0};
    std::atomic<int> flag{0};
    int data = 0;
    bool ok = true;
    std::thread consumer([&] {
        for (int r = 1; r <= rounds; ++r) {
            while (flag.load(std::memory_order_acquire) != r) {
            }
            if (data != r) {
                ok = false;          // stale data after seeing the flag: forbidden
            }
            turn.store(r, std::memory_order_release);
        }
    });
    for (int r = 1; r <= rounds; ++r) {
        data = r;                                    // plain write ...
        flag.store(r, std::memory_order_release);    // ... published by the release
        while (turn.load(std::memory_order_acquire) != r) {
        }
    }
    consumer.join();
    return ok;
}

uint64_t parallel_count(int threads, int each)
{
    std::atomic<uint64_t> n{0};
    std::thread t[4];
    for (int i = 0; i < threads; ++i) {
        t[i] = std::thread([&] {
            for (int k = 0; k < each; ++k) {
                n.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (int i = 0; i < threads; ++i) {
        t[i].join();
    }
    return n.load();
}

} // namespace

int main()
{
    std::printf("arch-neutral suite on %s\n", ARCH_NAME);
    std::printf("facts (allowed to differ):\n");
    std::printf("  byte order         %s\n", std::endian::native == std::endian::little ? "little-endian" : "big-endian");
    std::printf("  sizeof(void*)      %zu, sizeof(long) %zu\n", sizeof(void*), sizeof(long));
    std::printf("  plain char is      %s\n", std::numeric_limits<char>::is_signed ? "signed" : "unsigned");
    std::printf("  page size          %ld (sysconf)\n", sysconf(_SC_PAGESIZE));
    std::printf("  64-bit atomics     %s\n", std::atomic<uint64_t>::is_always_lock_free ? "always lock-free" : "not always lock-free");
    std::printf("  max_align_t        %zu bytes\n", alignof(std::max_align_t));

    std::printf("tests (must pass everywhere):\n");
    const uint8_t ext2_magic[2] = {0x53, 0xef};            // on disk: little-endian 0xEF53
    const uint8_t dtb_magic[4] = {0xd0, 0x0d, 0xfe, 0xed};  // in a DTB: big-endian 0xD00DFEED
    const uint8_t port[2] = {0x1f, 0x90};                   // network order: 8080
    const uint8_t blocks[4] = {0x00, 0x00, 0x01, 0x00};     // little-endian 65536
    check(le16(ext2_magic) == 0xef53, "explicit little-endian read of an ext2 magic");
    check(be32(dtb_magic) == 0xd00dfeedu, "explicit big-endian read of a DTB magic");
    check(be16(port) == 8080 && le32(blocks) == 65536, "network-order and on-disk 32-bit fields");

    const uint64_t page = static_cast<uint64_t>(sysconf(_SC_PAGESIZE));
    bool pages_ok = true;
    for (uint64_t p : {page, uint64_t{4096}, uint64_t{16384}, uint64_t{65536}}) {
        pages_ok = pages_ok && pages_for(256u << 20, p) * p == (256u << 20) &&
                   align_down(0x40001234, p) % p == 0 && align_down(0x40001234, p) <= 0x40001234;
    }
    check(pages_ok, "page arithmetic with the page size as a parameter (4, 16, 64 KiB)");

    const char bytes[] = "caf\xc3\xa9";                     // "café" in UTF-8: two high bytes
    int high = 0;
    for (const char* c = bytes; *c != '\0'; ++c) {
        high += is_high_byte(*c) ? 1 : 0;
    }
    check(high == 2, "byte classification through unsigned char");

    check(message_passing_rounds(20000), "release/acquire message passing, 20000 rounds");
    check(parallel_count(4, 50000) == 200000, "atomic fetch_add from 4 threads");
    check(sizeof(uintptr_t) == sizeof(void*), "pointers stored only in uintptr_t");

    std::printf("%d passed, %d failed on %s\n", g_pass, g_fail, ARCH_NAME);
    return g_fail == 0 ? 0 : 1;
}
