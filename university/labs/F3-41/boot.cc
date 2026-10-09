// boot.cc - F3-41 Listing 6: an A/B boot loader for the emulated lab board. It checks both
// slots (magic, size, SHA-256 of the payload), boots the valid image with the highest
// version, and stays in a recovery loop if neither is valid. A real secure boot loader
// (MCUboot) also verifies a signature here; see the chapter for what that adds.
#include "board.h"
#include "image.h"
#include "sha256.h"

namespace {

bool slotValid(uint32_t base, const slots::Header*& out)
{
    const auto* h = reinterpret_cast<const slots::Header*>(base);
    board::print("slot at "); board::printHex(base); board::print(": ");
    if (h->magic != slots::kMagic) {
        board::print("no image (magic "); board::printHex(h->magic); board::print(")\n");
        return false;
    }
    if (h->payloadSize == 0 || h->payloadSize > slots::kSlotSize - slots::kHeaderSize ||
        h->loadAddress != base + slots::kHeaderSize) {
        board::print("bad header\n");
        return false;
    }
    Sha256 sha;
    sha.update(reinterpret_cast<const uint8_t*>(base + slots::kHeaderSize), h->payloadSize);
    uint8_t digest[32];
    sha.finish(digest);
    for (int i = 0; i < 32; ++i) {
        if (digest[i] != h->sha256[i]) {
            board::print("version "); board::printDec(h->version);
            board::print(", HASH MISMATCH (image damaged)\n");
            return false;
        }
    }
    board::print("version "); board::printDec(h->version); board::print(", hash ok\n");
    out = h;
    return true;
}

[[noreturn]] void jumpTo(uint32_t vectors)
{
    const uint32_t* v = reinterpret_cast<const uint32_t*>(vectors);
    board::reg(0xE000ED08) = vectors;          // VTOR: the application's vector table
    asm volatile("dsb\n isb" ::: "memory");
    asm volatile("msr msp, %0\n bx %1" : : "r"(v[0]), "r"(v[1]));   // its stack, its reset
    for (;;) {
    }
}

}  // namespace

int main()
{
    board::uartInit();
    board::print("F3-41 boot loader: checking slots\n");
    const slots::Header* a = nullptr;
    const slots::Header* b = nullptr;
    const bool okA = slotValid(slots::kSlotA, a);
    const bool okB = slotValid(slots::kSlotB, b);
    const slots::Header* pick = nullptr;
    if (okA && okB) { pick = b->version > a->version ? b : a; }
    else if (okA) { pick = a; }
    else if (okB) { pick = b; }
    if (pick == nullptr) {
        board::print("no valid image: staying in recovery mode (waiting for a new image)\n");
        board::exitEmulator(false);
    }
    board::print("booting version "); board::printDec(pick->version);
    board::print(" at "); board::printHex(pick->loadAddress); board::print("\n");
    jumpTo(pick->loadAddress);
}
