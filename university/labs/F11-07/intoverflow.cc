// intoverflow.cc - F11-07, demo 3: an integer overflow becomes a heap overflow.
// A classic allocation bug: compute size = count * width in a 32-bit type, then
// allocate that many bytes and write `count` items. When count * width wraps
// past 2^32, the buffer is far too small and the loop writes off the end.
// AddressSanitizer catches THIS one (it is a real out-of-bounds write on a heap
// allocation), unlike the intra-object overflow of adjacent.cc. Built with
// -fsanitize=address by run.sh; the run shows the ASan report.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv)
{
    // count comes from "the network": here from argv so the run is reproducible.
    std::uint32_t count = (argc > 1) ? static_cast<std::uint32_t>(std::strtoul(argv[1], nullptr, 10))
                                     : 0x40000001u;     // 2^30 + 1
    const std::uint32_t width = 4;                      // 4 bytes per item

    // The bug: this multiply is 32-bit and wraps. 0x40000001 * 4 = 0x100000004,
    // truncated to 0x00000004 -> a 4-byte buffer for a billion items.
    std::uint32_t bytes = count * width;
    std::printf("count=%u width=%u -> bytes=%u (wrapped!)\n", count, width, bytes);

    unsigned char* buf = static_cast<unsigned char*>(std::malloc(bytes ? bytes : 1));
    if (!buf) { std::printf("malloc failed\n"); return 1; }

    // Write `count` items of `width` bytes: this walks far past the allocation.
    for (std::uint32_t i = 0; i < count; ++i) {
        std::memset(buf + static_cast<std::size_t>(i) * width, 0xcc, width);   // out of bounds almost at once
    }

    std::free(buf);
    std::printf("done (if you see this, the overflow was not detected)\n");
    return 0;
}
