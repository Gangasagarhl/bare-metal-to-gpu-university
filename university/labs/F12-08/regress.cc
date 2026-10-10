// regress.cc - the fuzzer's crash, kept forever as a regression test: a type-2 frame whose
// length byte (0x30 = 48) is larger than the 32-byte text array, plus three neighbours of the
// boundary. Built once against pkt.cc (must fail) and once against pkt_fixed.cc (must pass).
#include "pkt.hh"

#include <cstdio>
#include <vector>

namespace {

std::vector<std::uint8_t> text_frame(std::size_t len)
{
    std::vector<std::uint8_t> v{'S', 'P', 2, static_cast<std::uint8_t>(len)};
    unsigned sum = 0;
    for (std::size_t i = 0; i < len; ++i) {
        v.push_back('a');
        sum += 'a';
    }
    v.push_back(static_cast<std::uint8_t>(sum & 0xFF));
    return v;
}

}  // namespace

int main()
{
    int failed = 0;
    const struct {
        std::size_t len;
        bool valid;
    } cases[] = {{48, false}, {32, false}, {31, true}, {0, true}};
    for (const auto& c : cases) {
        const auto frame = text_frame(c.len);
        Frame f;
        const bool ok = parse_frame(frame.data(), frame.size(), f);
        const bool pass = ok == c.valid;
        failed += pass ? 0 : 1;
        std::printf("regress: text length %2zu -> %s (%s)\n", c.len, ok ? "accepted" : "refused",
                    pass ? "pass" : "FAIL");
    }
    return failed == 0 ? 0 : 1;
}
