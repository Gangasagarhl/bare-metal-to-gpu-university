// chain_model.h - F11-01 Listing 1: a hash-locked chain of trust, modelled on the host.
// Four boot stages (bl1, bl2, os, app) sit in "flash"; an immutable boot ROM holds the
// SHA-256 digest of bl1, and every stage holds the digest of the stage after it. Each stage
// checks the next one before handing over. A teaching model: real stages hold program bytes,
// and real chains use signatures (F11-02, F11-04) rather than fixed digests.
#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "sha256.h"

using Digest = std::array<uint8_t, 32>;

struct Stage {
    std::string name;
    std::string code;        // stands for the stage's program bytes
    Digest expected_next{};  // digest of the next stage, built into this stage
    bool checks_next = true; // false = a build in which the check was left out
};

inline Digest digest_of(const Stage& s)
{
    // the digest covers everything in the stage: code, embedded digest, and the check flag
    Sha256 h;
    h.update(reinterpret_cast<const uint8_t*>(s.code.data()), s.code.size());
    h.update(s.expected_next.data(), s.expected_next.size());
    const uint8_t flag = s.checks_next ? 1 : 0;
    h.update(&flag, 1);
    Digest d{};
    h.finish(d.data());
    return d;
}

inline std::string short_hex(const Digest& d)
{
    std::string s;
    for (int i = 0; i < 4; ++i) {
        s += "0123456789abcdef"[d[i] >> 4];
        s += "0123456789abcdef"[d[i] & 0xf];
    }
    return s;
}

// The vendor's release process: digests are filled in from the last stage backwards.
inline std::vector<Stage> build_release(const std::string& app_code, bool bl2_checks = true)
{
    std::vector<Stage> chain = {{"bl1", "bl1 v1: set up memory, load bl2"},
                                {"bl2", "bl2 v1: load os"},
                                {"os", "os v1: start drivers, start app"},
                                {"app", app_code}};
    chain[1].checks_next = bl2_checks;
    for (int i = static_cast<int>(chain.size()) - 2; i >= 0; --i) {
        chain[i].expected_next = digest_of(chain[i + 1]);
    }
    return chain;
}

// Boot: the ROM checks bl1; then every stage that checks, checks its successor.
// Returns true when all stages ran. With verbose = false nothing is printed.
inline bool boot(const Digest& rom_expected, const std::vector<Stage>& chain, bool verbose = true)
{
    Digest expected = rom_expected;
    std::string checker = "ROM";
    std::string broken_at;          // first stage that ran without checking its successor
    for (const Stage& s : chain) {
        const Digest actual = digest_of(s);
        if (verbose) {
            std::cout << "    " << checker << " -> " << s.name << ": digest " << short_hex(actual);
        }
        if (!broken_at.empty()) {
            if (verbose) {
                std::cout << "  NOT CHECKED (chain broken at " << broken_at << "), runs\n";
            }
        } else if (actual == expected) {
            if (verbose) {
                std::cout << "  matches " << short_hex(expected) << ", runs\n";
            }
        } else {
            if (verbose) {
                std::cout << "  expected " << short_hex(expected) << "  REFUSED: boot stops\n";
            }
            return false;
        }
        if (broken_at.empty() && !s.checks_next) {
            broken_at = s.name;
        }
        expected = s.expected_next;
        checker = s.name;
    }
    if (verbose) {
        std::cout << "    result: all four stages ran\n";
    }
    return true;
}
