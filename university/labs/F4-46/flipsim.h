// flipsim.h - DR405 F4-46: the university's model of a display engine scanning out one
// buffer line by line, and of page flips that change the scanout address either at the
// start of vertical blank (synchronised) or at once (asynchronous). The mode is the
// 640x480 timing in QEMU's EDID (edid_test output): pixel clock 32,140 kHz, htotal 864,
// vtotal 496, vactive 480. This is a model of the idea, not a driver and not a measurement.
#pragma once
#include <cstdio>
#include <utility>
#include <vector>

namespace flip {
struct Mode { double clock_khz; int htotal, vtotal, vactive; };
constexpr Mode kQemu640{32140.0, 864, 496, 480};

inline double line_us(const Mode& m) { return m.htotal / (m.clock_khz / 1000.0); }
inline double frame_us(const Mode& m) { return line_us(m) * m.vtotal; }

struct Refresh { int index; double start_us; std::vector<std::pair<int, int>> parts; };  // (first line, buffer)
struct Flip { int buffer; double request_us; int line_at_request; double latched_us; int line_at_latch; bool dropped; };

// 'ready[i]' is the time at which buffer i+1 has finished rendering and its flip is asked for.
inline void run(const Mode& m, const std::vector<double>& ready, bool sync, int refreshes,
                std::vector<Refresh>& rows, std::vector<Flip>& flips)
{
    const double F = frame_us(m), L = line_us(m);
    int shown = 0, pending = -1;                      // buffer 0 is on screen at time 0
    Flip pending_flip{};
    size_t next = 0;
    for (int k = 0; k < refreshes; ++k) {
        const double start = k * F, vblank = start + m.vactive * L, end = start + F;
        Refresh r{k, start, {{0, shown}}};
        if (sync) {
            // every request made before this vertical blank is latched at its start
            while (next < ready.size() && ready[next] < vblank) {
                const double t = ready[next];
                if (pending >= 0) {                    // a newer flip replaces an older pending one
                    pending_flip.dropped = true;
                    flips.push_back(pending_flip);
                }
                pending = static_cast<int>(next) + 1;
                pending_flip = Flip{pending, t, static_cast<int>((t - F * static_cast<int>(t / F)) / L), vblank,
                                    m.vactive, false};
                ++next;
            }
            if (pending >= 0) {
                shown = pending;
                flips.push_back(pending_flip);
                pending = -1;
            }
        } else {
            // every request changes the scanout address at once, in the middle of a frame or not
            while (next < ready.size() && ready[next] < end) {
                const double t = ready[next];
                const int buf = static_cast<int>(next) + 1;
                const int line = static_cast<int>((t - start) / L);
                if (line > 0 && line < m.vactive) r.parts.push_back({line, buf});  // a tear at 'line'
                shown = buf;
                flips.push_back(Flip{buf, t, line, t, line, false});
                ++next;
            }
        }
        rows.push_back(r);
    }
}

inline void print(const Mode& m, const std::vector<Refresh>& rows, const std::vector<Flip>& flips)
{
    std::printf("  buffer  requested_us  line_at_request  latched_us  line_at_latch  in_blank\n");
    for (const Flip& f : flips) {
        if (f.dropped)
            std::printf("  %6d  %12.1f  %15d  %10s  %13s  replaced by a newer flip before latching\n", f.buffer,
                        f.request_us, f.line_at_request, "-", "-");
        else
            std::printf("  %6d  %12.1f  %15d  %10.1f  %13d  %s\n", f.buffer, f.request_us, f.line_at_request,
                        f.latched_us, f.line_at_latch, f.line_at_latch >= m.vactive ? "yes" : "NO");
    }
    int torn = 0;
    std::printf("  refresh  start_us  visible lines 0..%d show\n", m.vactive - 1);
    for (const Refresh& r : rows) {
        std::printf("  %7d  %8.1f ", r.index, r.start_us);
        for (size_t i = 0; i < r.parts.size(); ++i) {
            const int last = i + 1 < r.parts.size() ? r.parts[i + 1].first - 1 : m.vactive - 1;
            std::printf(" buffer %d (lines %d-%d)", r.parts[i].second, r.parts[i].first, last);
        }
        if (r.parts.size() > 1) {
            ++torn;
            std::printf("  TEAR");
        }
        std::printf("\n");
    }
    std::printf("  torn refreshes: %d of %zu\n", torn, rows.size());
}
}  // namespace flip
