// safe.cpp - F11-07, the fixed versions of all three demos, built and run by
// run_lab.sh with -fsanitize=address,undefined. It must run cleanly: the point
// of the chapter is that each attack disappears once the length is checked.
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <limits>

namespace {
int failures = 0;

// Fix for adjacent.cc / hijack.cc: a bounded, typed name. std::string owns its
// own storage; there is no fixed field to overflow into a neighbour.
struct Account {
    std::string user;
    bool is_admin = false;
};

void set_name(Account& a, const std::string& src)
{
    a.user = src;                 // grows as needed; cannot touch is_admin
}

// Fix for intoverflow.cc: do the multiply in a wide type with an overflow-safe
// check AND bound the request to a sane maximum. Untrusted input must never be
// allowed to ask for an unbounded allocation, even when the arithmetic is sound.
constexpr std::size_t kMaxItems = 1u << 20;      // one million items, our policy
std::vector<unsigned char>* make_buffer(std::uint32_t count, std::uint32_t width)
{
    if (count > kMaxItems) return nullptr;                                   // bound the size
    if (width != 0 && count > std::numeric_limits<std::size_t>::max() / width) return nullptr;
    return new std::vector<unsigned char>(static_cast<std::size_t>(count) * width, 0);
}
}  // namespace

int main()
{
    Account a;
    set_name(a, std::string(1000, 'A'));      // a huge name: no corruption
    if (a.is_admin) { std::printf("FAIL: is_admin flipped\n"); ++failures; }
    else            std::printf("ok: 1000-byte name, is_admin still false\n");

    auto* big = make_buffer(0x40000001u, 4);  // the overflowing request
    if (big != nullptr) { std::printf("FAIL: oversized request accepted\n"); ++failures; delete big; }
    else                std::printf("ok: overflowing count*width rejected\n");

    auto* okbuf = make_buffer(16, 4);         // a normal request
    if (okbuf == nullptr || okbuf->size() != 64) { std::printf("FAIL: valid request rejected\n"); ++failures; }
    else                std::printf("ok: valid request allocated %zu bytes\n", okbuf->size());
    delete okbuf;

    std::printf(failures ? "SOME TESTS FAILED\n" : "ALL TESTS PASSED\n");
    return failures;
}
