// HW204 course project, reference solution in the course's simulators (Lab Engineer, guide
// 11.5). The real project runs on the kit's microcontroller (Raspberry Pi Pico 2 / RP2350 or
// NUCLEO-F446RE) with the LSM6DSOX breakout and an FX2 logic analyser; that version is
// UNTESTED ON HARDWARE (no board in this build, rulings C3/D1). This program proves the
// structure is feasible and records the numbers: a timer interrupt at a computed rate starts
// an I2C read of a 6-byte sensor record (the course's I2C model from the practical), the
// completion pushes the record into a single-producer ring buffer, and the main loop prints
// readings with sequence numbers over a modelled UART. Exercise values throughout.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "i2c_exam_model.h"

constexpr double kFin = 16e6;           // exercise clock (not the RP2350's or STM32F446's)
constexpr int kRateHz = 200;            // sampling rate wanted
constexpr int kUsPerTick = 5000;        // one timer tick = 5 ms at 200 Hz

struct Reading { std::uint32_t seq; std::uint32_t t_ms; std::uint8_t raw[6]; };

struct Ring                              // single producer (handler), single consumer (main)
{
    std::vector<Reading> slots;
    std::size_t head = 0, tail = 0, high = 0;
    long dropped = 0;
    explicit Ring(std::size_t n) : slots(n) {}
    std::size_t count() const { return (head + slots.size() - tail) % slots.size(); }
    void push(const Reading& r)
    {
        if (count() == slots.size() - 1) { ++dropped; return; }
        slots[head] = r; head = (head + 1) % slots.size(); high = std::max(high, count());
    }
    bool pop(Reading& r)
    {
        if (head == tail) return false;
        r = slots[tail]; tail = (tail + 1) % slots.size(); return true;
    }
};

static bool deadline_passed(std::uint32_t now, std::uint32_t deadline)   // wrap-safe (F1-48)
{
    return static_cast<std::int32_t>(now - deadline) >= 0;
}

int main()
{
    // 1. Timer settings (F1-48): smallest prescaler whose reload fits 16 bits.
    std::printf("timer: f_in %.0f Hz, wanted %d Hz\n", kFin, kRateHz);
    int chosen_p = 0; long chosen_reload = 0;
    for (int p : {1, 8, 64, 256}) {
        const double counts = kFin / p / kRateHz;
        const long reload = std::lround(counts) - 1;
        const double actual = kFin / p / (reload + 1);
        std::printf("  prescaler %3d: counts %.2f reload %ld fits %s actual %.3f Hz error %+.4f %%\n", p, counts, reload,
                    reload <= 65535 ? "yes" : "no", actual, (actual - kRateHz) / kRateHz * 100);
        if (chosen_p == 0 && reload <= 65535) { chosen_p = p; chosen_reload = reload; }
    }
    std::printf("  chosen: prescaler %d, reload %ld (period %.3f ms)\n", chosen_p, chosen_reload, (chosen_reload + 1) * chosen_p / kFin * 1e3);

    // 2. The sensor on the course's I2C model: 6 bytes from register 0x00 (two per axis; exercise values).
    Bus bus;
    Target sensor{0x5A, {0x11, 0x22, 0x33, 0x44}};
    bus.targets.push_back(&sensor);
    Controller c(bus);
    Ring ring(8);
    std::uint32_t seq = 0;
    std::uint32_t now_ms = 0xFFFFFFFFu - 2000u;   // start near the wrap to exercise the deadline check
    std::uint32_t next_tick = now_ms;
    long printed = 0, uart_bytes = 0, missing = 0;
    std::uint32_t last_seq = 0;
    std::vector<std::size_t> read_start_sample;
    const int kTicks = 10 * 60 * kRateHz;         // ten minutes at 200 Hz
    for (int tick = 0; tick < kTicks; ++tick) {
        // timer interrupt: start the I2C read (the model completes it at once; on hardware the
        // I2C peripheral's interrupts or a DMA transfer do, and the completion handler pushes).
        if (deadline_passed(now_ms, next_tick)) {
            read_start_sample.push_back(bus.trace.size());
            std::vector<std::uint8_t> raw;
            Reading r{++seq, now_ms, {}};
            read_reg(c, 0x5A, 0x00, 6, raw);
            for (int i = 0; i < 6; ++i) r.raw[i] = raw[i];
            ring.push(r);
            next_tick += kUsPerTick / 1000;
            if (bus.trace.size() > 4096) bus.trace.erase(bus.trace.begin(), bus.trace.end() - 2048);  // keep the model small
        }
        // main loop: every 7th tick it is busy (display refresh) and drains nothing; otherwise it prints.
        if (tick % 7 != 6) {
            Reading r;
            while (ring.pop(r)) {
                char line[64];
                const int n = std::snprintf(line, sizeof line, "%u,%u,%02X%02X,%02X%02X,%02X%02X\n", r.seq, r.t_ms,
                                            r.raw[0], r.raw[1], r.raw[2], r.raw[3], r.raw[4], r.raw[5]);
                uart_bytes += n;
                if (printed < 3) std::printf("  UART> %s", line);
                if (last_seq && r.seq != last_seq + 1) missing += r.seq - last_seq - 1;
                last_seq = r.seq; ++printed;
            }
        }
        now_ms += kUsPerTick / 1000;
    }
    std::printf("run: %d ticks (%d minutes at %d Hz), %u readings taken, %ld printed, %ld missing sequence numbers, ring dropped %ld, ring high water %zu of %zu\n",
                kTicks, kTicks / kRateHz / 60, kRateHz, seq, printed, missing, ring.dropped, ring.high, ring.slots.size() - 1);
    std::printf("UART: %ld bytes in %.0f s = %.0f bytes/s; at 115200 baud 8N1 (11520 bytes/s max) the line is %.1f %% busy\n",
                uart_bytes, kTicks / static_cast<double>(kRateHz), uart_bytes / (kTicks / static_cast<double>(kRateHz)),
                100.0 * uart_bytes / (kTicks / static_cast<double>(kRateHz)) / 11520.0);
    std::printf("time: millisecond counter started at %u and wrapped during the run (ends at %u); wrap-safe deadline check used\n",
                0xFFFFFFFFu - 2000u, now_ms);

    // 3. The "logic-analyser trace": one transaction from the model, decoded, and the spacing.
    Bus one; Target s2{0x5A, {0x11, 0x22, 0x33, 0x44}}; one.targets.push_back(&s2); Controller c2(one);
    std::vector<std::uint8_t> raw; read_reg(c2, 0x5A, 0x00, 6, raw);
    std::printf("trace: one read is %zu samples = %.2f bit times; at 100 kbit/s (I2C standard mode, UM10204 section 5) that is %.3f ms per read, %.1f %% of the %.1f ms period\n",
                one.trace.size(), one.trace.size() / 4.0, one.trace.size() / 4.0 / 100e3 * 1e3,
                100.0 * one.trace.size() / 4.0 / 100e3 / (1.0 / kRateHz), 1000.0 / kRateHz);
    std::printf("trace: the first transaction as the analyser would decode it:\n");
    print_capture(one.trace);
    std::printf("  START, 0x%02X (0x5A W) ACK, 0x00 ACK, START, 0x%02X (0x5A R) ACK,", 0x5A << 1, (0x5A << 1) | 1);
    for (std::size_t i = 0; i < raw.size(); ++i) std::printf(" 0x%02X %s,", raw[i], i + 1 < raw.size() ? "ACK" : "NACK");
    std::printf(" STOP\n");
    std::printf("trace: spacing between the starts of consecutive reads in the model = one timer period = %d ms (on hardware: measure it on the trace and compare with the %.3f ms computed above)\n",
                kUsPerTick / 1000, (chosen_reload + 1) * chosen_p / kFin * 1e3);
    return (missing == 0 && ring.dropped == 0) ? 0 : 1;
}
