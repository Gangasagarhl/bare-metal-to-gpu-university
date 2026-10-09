// F1-43: a toy interrupt controller with 8 input lines, in the style of the
// classic PC controller: a pending register (IRR), a mask register (IMR) and an
// in-service register (ISR). Lower line number = higher priority in this model.
// The controller forwards the best pending, unmasked line to the CPU only if no
// line of equal or higher priority is still in service; the handler's
// end-of-interrupt (EOI) command clears the in-service bit.
#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

struct Controller
{
    std::uint8_t irr = 0;   // requested, not yet accepted
    std::uint8_t imr = 0;   // 1 = masked (blocked)
    std::uint8_t isr = 0;   // accepted by the CPU, handler not finished
    int vector_base = 32;   // vector number for line 0

    void raise(int line) { irr |= static_cast<std::uint8_t>(1u << line); }

    // Which line would be forwarded now? -1 if none.
    int best() const
    {
        for (int l = 0; l < 8; ++l) {
            const std::uint8_t bit = static_cast<std::uint8_t>(1u << l);
            if (isr & bit) {
                return -1;            // a line at least this important is in service
            }
            if ((irr & bit) && !(imr & bit)) {
                return l;
            }
        }
        return -1;
    }
    int acknowledge(int line)        // CPU accepts: IRR bit -> ISR bit, returns vector
    {
        const std::uint8_t bit = static_cast<std::uint8_t>(1u << line);
        irr = static_cast<std::uint8_t>(irr & ~bit);
        isr |= bit;
        return vector_base + line;
    }
    void eoi()                        // clear the highest-priority in-service bit
    {
        for (int l = 0; l < 8; ++l) {
            const std::uint8_t bit = static_cast<std::uint8_t>(1u << l);
            if (isr & bit) {
                isr = static_cast<std::uint8_t>(isr & ~bit);
                return;
            }
        }
    }
};

inline std::string bits(std::uint8_t v)
{
    std::string s;
    for (int l = 7; l >= 0; --l) {
        s += ((v >> l) & 1u) ? '1' : '0';
    }
    return s;
}

struct Handler
{
    const char* name;
    int length;        // ticks the handler runs
};

// Run a scripted scenario. raise_at[t] lists the lines raised at tick t.
// `forget_eoi_line` names a handler that returns without sending EOI (-1: none).
template <std::size_t T>
std::array<int, 8> simulate(const std::array<std::uint8_t, T>& raise_at,
                            const std::array<Handler, 8>& h, Controller& c,
                            int forget_eoi_line, bool verbose)
{
    std::array<int, 8> served{};
    // A stack of running handlers: line and ticks left (nesting allowed).
    std::array<int, 8> stack_line{};
    std::array<int, 8> stack_left{};
    int depth = 0;
    for (std::size_t t = 0; t < T; ++t) {
        for (int l = 0; l < 8; ++l) {
            if (raise_at[t] & (1u << l)) {
                c.raise(l);
                if (verbose) {
                    std::printf("t=%2zu  %-6s raises line %d        IRR=%s IMR=%s ISR=%s\n", t,
                                h[l].name, l, bits(c.irr).c_str(), bits(c.imr).c_str(),
                                bits(c.isr).c_str());
                }
            }
        }
        const int l = c.best();
        if (l >= 0) {
            const int vec = c.acknowledge(l);
            stack_line[depth] = l;
            stack_left[depth] = h[l].length;
            ++depth;
            ++served[l];
            if (verbose) {
                std::printf("t=%2zu  CPU takes line %d (vector %d): %s handler%s\n", t, l, vec,
                            h[l].name, depth > 1 ? "  [nested: preempts the running one]" : "");
            }
        }
        if (depth > 0 && --stack_left[depth - 1] == 0) {
            const int done = stack_line[depth - 1];
            --depth;
            if (done == forget_eoi_line) {
                if (verbose) {
                    std::printf("t=%2zu  %s handler returns WITHOUT EOI        ISR=%s\n", t,
                                h[done].name, bits(c.isr).c_str());
                }
            } else {
                c.eoi();
                if (verbose) {
                    std::printf("t=%2zu  %s handler sends EOI and returns    ISR=%s\n", t,
                                h[done].name, bits(c.isr).c_str());
                }
            }
        }
    }
    return served;
}
