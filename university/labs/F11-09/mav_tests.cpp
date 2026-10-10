// mav_tests.cpp - unit tests for the FIXED MAVLink-style scanner. Built and run
// by run_lab.sh under -fsanitize=address,undefined. The fixed scanner must
// never read past the buffer, however the length byte lies.
#define FIXED
#include "mav_target.cc"      // brings in fuzz_one (the fixed version)

#include <cstdio>
#include <vector>

namespace {
void feed(const char* what, std::vector<unsigned char> in)
{
    fuzz_one(in.data(), in.size());
    std::printf("ok: %s (%zu bytes scanned without over-read)\n", what, in.size());
}
}  // namespace

int main()
{
    feed("empty", {});
    feed("stx then nothing", {0xFE});
    feed("stx, len=255, but truncated", {0xFE, 0xFF, 0x00});
    feed("stx, len=10, only 2 payload bytes", {0xFE, 0x0A, 0, 1, 1, 0, 0x01, 0x02});
    feed("garbage only", std::vector<unsigned char>(32, 0x00));
    // a complete minimal frame followed by a truncated one
    feed("valid frame + truncated frame",
         {0xFE, 0x00, 0, 1, 1, 0, 0x11, 0x22,   // len 0 frame (header + 2 checksum)
          0xFE, 0x20, 0x01});                    // then a lying len=32 with no payload
    std::printf("ALL TESTS PASSED\n");
    return 0;
}
