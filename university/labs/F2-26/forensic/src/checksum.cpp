#include "checksum.h"

std::uint32_t checksum(const std::string& text)
{
    std::uint32_t h = 0;
    for (const char c : text) {
        h = h * 31u + static_cast<unsigned char>(c);
    }
    return h;
}
