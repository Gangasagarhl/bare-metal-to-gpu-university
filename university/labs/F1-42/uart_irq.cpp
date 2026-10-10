// F1-42 Listing 2: the same echo with a receive interrupt and a ring buffer.
// When a byte arrives, the interrupt handler runs at once (it preempts the main
// loop), moves the byte into a ring buffer and returns. The main loop still
// does `work` ticks of other jobs, then drains the ring buffer.
#include <array>
#include <cstdio>
#include <string>

#include "uart_model.h"

template <std::size_t N>
struct Ring                       // single producer (handler), single consumer (main)
{
    std::array<std::uint8_t, N> buf{};
    std::size_t head = 0;         // next slot to write (handler)
    std::size_t tail = 0;         // next slot to read (main loop)
    std::size_t count = 0;
    std::size_t high_water = 0;
    long dropped = 0;

    void push(std::uint8_t b)
    {
        if (count == N) {
            ++dropped;            // buffer full: the handler must drop (and count) it
            return;
        }
        buf[head] = b;
        head = (head + 1) % N;
        ++count;
        if (count > high_water) {
            high_water = count;
        }
    }
    bool pop(std::uint8_t& b)
    {
        if (count == 0) {
            return false;
        }
        b = buf[tail];
        tail = (tail + 1) % N;
        --count;
        return true;
    }
};

// work > 0: constant work per pass; work == 0: the field workload of uart_model.h
template <std::size_t N>
void run(int work)
{
    Uart u;
    Ring<N> ring;
    Checksum echo;
    long echoed = 0;
    long interrupts = 0;
    int sent = 0;
    int next_drain = 0;
    long pass = 0;
    for (int t = 0; sent < kTotal || ring.count > 0; ++t) {
        if (sent < kTotal && t % kGap == 0) {
            u.deliver(pattern(sent));
            ++sent;
            ++interrupts;                    // "data ready" raises the interrupt line
            while (u.data_ready()) {         // the handler: empty the UART, then return
                ring.push(u.read());
            }
        }
        if (t >= next_drain) {               // main loop: drain, then other jobs
            std::uint8_t b = 0;
            while (ring.pop(b)) {
                echo.add(b);
                ++echoed;
            }
            next_drain = t + (work > 0 ? work : field_work(pass));
            ++pass;
        }
    }
    const bool ok = echo.value() == expected_checksum();
    const std::string load = work > 0 ? std::to_string(work) : "field";
    std::printf("ring %3zu  work %5s  interrupts %6ld  echoed %6ld  uart lost %ld  "
                "ring dropped %5ld  high water %3zu  checksum 0x%08X  %s\n",
                N, load.c_str(), interrupts, echoed, u.lost, ring.dropped, ring.high_water,
                static_cast<unsigned>(echo.value()), ok ? "PASS" : "FAIL");
}

int main()
{
    std::printf("sender: %d bytes, one every %d ticks; expected checksum 0x%08X\n", kTotal, kGap,
                static_cast<unsigned>(expected_checksum()));
    run<64>(0);       // the forensic lab's field workload, with the fix
    run<2>(0);        // the same, with a ring that is too small
    run<64>(25);      // constant heavy load
    run<64>(300);     // much heavier load, same ring
    run<2>(300);
    return 0;
}
