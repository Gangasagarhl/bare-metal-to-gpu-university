// secded.cpp - F1-54 Listing 1: a (72,64) Hamming code with an extra overall parity bit
// (SECDED: single-error correction, double-error detection), tested exhaustively.
// Codeword bit 0 = overall parity; bits 1, 2, 4, 8, 16, 32, 64 = Hamming check bits;
// the other 64 positions (3, 5, 6, 7, 9, ... 71) hold the data bits.
#include <bitset>
#include <cstdint>
#include <cstdio>

using Word = std::bitset<72>;

static bool isPowerOfTwo(int p) { return (p & (p - 1)) == 0; }

static Word encode(std::uint64_t data)
{
    Word w;
    int d = 0;
    for (int pos = 1; pos < 72; ++pos) {
        if (!isPowerOfTwo(pos)) {
            w[static_cast<std::size_t>(pos)] = (data >> d++) & 1u;
        }
    }
    for (int p = 1; p < 72; p <<= 1) {         // each check bit covers positions with bit p set
        bool parity = false;
        for (int pos = 1; pos < 72; ++pos) {
            if ((pos & p) && pos != p) {
                parity ^= w[static_cast<std::size_t>(pos)];
            }
        }
        w[static_cast<std::size_t>(p)] = parity;
    }
    w[0] = w.count() % 2 == 1;                  // make the total number of ones even
    return w;
}

static std::uint64_t dataOf(const Word& w)
{
    std::uint64_t data = 0;
    int d = 0;
    for (int pos = 1; pos < 72; ++pos) {
        if (!isPowerOfTwo(pos)) {
            data |= static_cast<std::uint64_t>(w[static_cast<std::size_t>(pos)]) << d++;
        }
    }
    return data;
}

enum class Result { Clean, Corrected, Detected };

static Result decode(Word& w)
{
    int syndrome = 0;
    for (int pos = 1; pos < 72; ++pos) {
        if (w[static_cast<std::size_t>(pos)]) {
            syndrome ^= pos;                    // XOR of the positions of all ones
        }
    }
    const bool parityBad = w.count() % 2 == 1;
    if (syndrome == 0 && !parityBad) {
        return Result::Clean;
    }
    if (parityBad && syndrome < 72) {           // odd number of flips: assume one, fix it
        w.flip(static_cast<std::size_t>(syndrome));   // syndrome 0: the parity bit itself
        return Result::Corrected;
    }
    return Result::Detected;   // even number of flips, or a syndrome naming no real position
}

int main()
{
    const std::uint64_t data = 0x0123456789ABCDEFull;
    const Word clean = encode(data);
    std::printf("data 0x%016llx -> 72-bit codeword with %zu ones\n",
                static_cast<unsigned long long>(data), clean.count());

    int fixed = 0;
    for (std::size_t i = 0; i < 72; ++i) {
        Word w = clean;
        w.flip(i);
        if (decode(w) == Result::Corrected && dataOf(w) == data) {
            ++fixed;
        }
    }
    std::printf("single-bit errors: %d of 72 corrected\n", fixed);

    int detected = 0, pairs = 0;
    for (std::size_t i = 0; i < 72; ++i) {
        for (std::size_t j = i + 1; j < 72; ++j) {
            Word w = clean;
            w.flip(i);
            w.flip(j);
            ++pairs;
            if (decode(w) == Result::Detected) {
                ++detected;
            }
        }
    }
    std::printf("double-bit errors: %d of %d detected (none silently 'corrected')\n", detected, pairs);

    long triples = 0, wrong = 0, caught = 0;
    for (std::size_t i = 0; i < 72; ++i) {
        for (std::size_t j = i + 1; j < 72; ++j) {
            for (std::size_t k = j + 1; k < 72; ++k) {
                Word w = clean;
                w.flip(i);
                w.flip(j);
                w.flip(k);
                ++triples;
                const Result r = decode(w);
                if (r == Result::Corrected && dataOf(w) != data) {
                    ++wrong;
                } else if (r == Result::Detected) {
                    ++caught;
                }
            }
        }
    }
    std::printf("triple-bit errors: %ld of %ld 'corrected' into WRONG data, %ld detected\n",
                wrong, triples, caught);
    return 0;
}
