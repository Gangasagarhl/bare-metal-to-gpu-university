// f412_main.cc - DR302 F4-12 lab kernel (milestone C11): play a tone through HD Audio.
// Finds the controller, resets it, talks to the codec over CORB/RIRB, walks its widgets,
// then plays 2 s of a stereo test tone (left 440 Hz, right 660 Hz, 44.1 kHz, 16-bit)
// through a buffer descriptor list, one MSI per buffer. QEMU's "wav" audio backend
// records what the codec outputs; wavcheck.py measures it on the host.
// The forensic build (-DF412_RATE_BUG) gets the stream format's base-rate bit wrong.
#include "kbase.h"
#include "../F4-02/acpi.h"
#include "../F4-02/pci.h"
#include "../F4-08/intr.h"
#include "../F4-08/msi.h"
#include "../F4-08/dma.h"
#include "../F4-08/vtd.h"
#include "hda.h"

namespace {
// A 1024-point sine table, computed by the compiler (no floating point at run time:
// the kernel is built with -mgeneral-regs-only).
struct Table { int16_t v[1024]; };
consteval double sin_series(double x)            // x in [-pi, pi]; Taylor series, 12 terms
{
    double term = x, sum = x;
    for (int n = 1; n < 12; ++n) {
        term *= -x * x / ((2 * n) * (2 * n + 1));
        sum += term;
    }
    return sum;
}
consteval Table make_sine()
{
    Table t{};
    constexpr double pi = 3.14159265358979323846;
    for (int i = 0; i < 1024; ++i) {
        double x = 2 * pi * i / 1024;
        if (x > pi) x -= 2 * pi;
        const double s = sin_series(x) * 32767.0;
        t.v[i] = static_cast<int16_t>(s >= 0 ? s + 0.5 : s - 0.5);
    }
    return t;
}
constexpr Table SINE = make_sine();

constexpr uint32_t RATE = 44100, CHANNELS = 2, PERIODS = 8;
constexpr uint32_t PERIOD_BYTES = 44160;          // about 0.25 s; a multiple of 128 bytes
constexpr uint32_t BYTES = PERIOD_BYTES * PERIODS;
alignas(128) int16_t g_pcm[BYTES / 2];

// Phase accumulator: the top 10 bits of a 32-bit phase index the table.
void fill(uint32_t f_left, uint32_t f_right)
{
    const uint32_t inc_l = static_cast<uint32_t>((uint64_t{f_left} << 32) / RATE);
    const uint32_t inc_r = static_cast<uint32_t>((uint64_t{f_right} << 32) / RATE);
    uint32_t ph_l = 0, ph_r = 0;
    for (uint32_t i = 0; i < BYTES / 4; ++i) {
        g_pcm[2 * i] = static_cast<int16_t>(SINE.v[ph_l >> 22] / 2);      // -6 dB full scale
        g_pcm[2 * i + 1] = static_cast<int16_t>(SINE.v[ph_r >> 22] / 2);
        ph_l += inc_l;
        ph_r += inc_r;
    }
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-12 kernel: magic=0x%x\n", magic);
    intr::init();
    uint64_t ecam;
    uint8_t b0, b1;
    if (acpi::ecam(ecam, b0, b1)) pci::use_ecam(ecam, b0, b1);
    PciAddr a;
    if (!hda::find(a)) panic("no HD Audio controller");
    const bool iommu = dma::use_iommu();
    if (iommu) {
        dma::attach(a);
        vtd::enable();
    } else {
        dma::use_identity();
    }
    const uint8_t vec = intr::alloc_vector();
    intr::set_handler(vec, hda::irq);
    const bool have_msi = msi::enable(a, vec);
    kprintf("hda: interrupts by %s, DMA %s\n", have_msi ? "MSI" : "polling", iommu ? "through VT-d" : "identity");
    if (!hda::init(a, have_msi ? vec : 0)) panic("hda: no codec answered the reset");

    hda::Path path;
    uint8_t cad = 0;
    while (cad < 15 && !(hda::codecs() & (1u << cad))) ++cad;
    if (!hda::walk(cad, path)) panic("hda: no output path");

    fill(440, 660);
    kprintf("pcm: %u bytes = %u frames of %u-channel 16-bit at %u Hz (left 440 Hz, right 660 Hz), fnv1a 0x%08x\n", BYTES,
            BYTES / 4, CHANNELS, RATE, fnv1a(g_pcm, BYTES));
    const uint16_t fmt = hda::format(RATE, 16, CHANNELS);
    const hda::PlayStats st = hda::play(path, g_pcm, BYTES, fmt, PERIODS, PERIODS);
    kprintf("stream: %u of %u buffer completions in %u ms (PIT clock), %u interrupts, LPIB %u at stop, FIFO error %u\n",
            st.periods, PERIODS, static_cast<uint32_t>(st.ms), st.irqs, st.lpib_last, st.fifoe);
    const bool ok = st.periods == PERIODS && st.fifoe == 0;
    kprintf("F4-12 %s\n", ok ? "done" : "FAILED");
    qemu_exit(ok ? 0x10 : 1);
}
