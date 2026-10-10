// HW204 final exam, forensic key: the same board as uart_fifo_forensic.cpp after the fix
// (receive interrupt plus a ring buffer, as F1-42 Listing 2), under the same workload,
// plus the two "wrong" fixes candidates may propose: a deeper FIFO with polling.
// Lab Engineer's verification run; numbers go into the answer key.
#include <algorithm>
#include <cstdio>
#include <vector>

#include "uart_model.h"

static int board2_work(long pass) { return (pass % 16 == 15) ? 60 : 4; }

struct Ring
{
    std::vector<std::uint8_t> slots;
    std::size_t head = 0, tail = 0, count = 0, high = 0;
    long dropped = 0;
    explicit Ring(std::size_t n) : slots(n) {}
    void push(std::uint8_t b)
    {
        if (count == slots.size()) { ++dropped; return; }
        slots[head] = b; head = (head + 1) % slots.size(); ++count; high = std::max(high, count);
    }
    bool pop(std::uint8_t& b)
    {
        if (count == 0) return false;
        b = slots[tail]; tail = (tail + 1) % slots.size(); --count; return true;
    }
};

static void run_irq(std::size_t fifo, std::size_t ring_slots)
{
    Uart u; u.depth = fifo;
    Ring ring(ring_slots);
    Checksum echo; long echoed = 0, irqs = 0; int sent = 0, next_poll = 0; long pass = 0;
    for (int t = 0; sent < kTotal || u.data_ready() || ring.count > 0; ++t) {
        if (sent < kTotal && t % kGap == 0) {
            u.deliver(pattern(sent)); ++sent;
            ++irqs;                                  // the receive interrupt: the handler empties the UART
            while (u.data_ready()) ring.push(u.read());
        }
        if (t >= next_poll) {
            std::uint8_t b;
            while (ring.pop(b)) { echo.add(b); ++echoed; }
            next_poll = t + board2_work(pass); ++pass;
        }
    }
    std::printf("interrupt + ring %3zu  fifo %zu  interrupts %ld  echoed %ld  uart lost %ld  ring dropped %ld  high water %zu  checksum 0x%08X  %s\n",
                ring_slots, fifo, irqs, echoed, u.lost, ring.dropped, ring.high, static_cast<unsigned>(echo.value()),
                echo.value() == expected_checksum() ? "PASS" : "FAIL");
}

static void run_poll(std::size_t fifo)
{
    Uart u; u.depth = fifo;
    Checksum echo; long echoed = 0; int sent = 0, next_poll = 0; long pass = 0; std::size_t high = 0;
    for (int t = 0; sent < kTotal || u.data_ready(); ++t) {
        if (sent < kTotal && t % kGap == 0) { u.deliver(pattern(sent)); ++sent; high = std::max(high, u.rx.size()); }
        if (t >= next_poll) {
            while (u.data_ready()) { echo.add(u.read()); ++echoed; }
            next_poll = t + board2_work(pass); ++pass;
        }
    }
    std::printf("polling, fifo %2zu           echoed %ld  uart lost %ld  fifo high water %zu  checksum 0x%08X  %s\n",
                fifo, echoed, u.lost, high, static_cast<unsigned>(echo.value()),
                echo.value() == expected_checksum() ? "PASS" : "FAIL");
}

int main()
{
    std::printf("same workload as the bench (4-tick passes, 60 ticks every 16th pass), %d bytes, one per %d ticks; expected checksum 0x%08X\n",
                kTotal, kGap, static_cast<unsigned>(expected_checksum()));
    run_irq(4, 64);
    run_irq(4, 8);
    run_irq(4, 4);
    run_irq(1, 64);
    run_poll(4);
    run_poll(6);
    run_poll(7);
    run_poll(8);
    return 0;
}
