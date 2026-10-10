// hda.cc - DR302 F4-12: Intel HD Audio controller + codec (see hda.h).
#include "hda.h"
#include "kbase.h"
#include "../F4-08/intr.h"
#include "../F4-08/dma.h"

namespace {
PciAddr g_dev;
uintptr_t g_base;
uint16_t g_codecs;
uint32_t g_iss, g_oss;
bool g_use_irq;

// CORB: 256 commands of 4 bytes; RIRB: 256 responses of 8 bytes (response, extended).
// Both must be 128-byte aligned; the BDL too.
alignas(128) uint32_t g_corb[256];
alignas(128) uint32_t g_rirb[512];
struct Bde { uint32_t addr_lo, addr_hi, length, ioc; };
alignas(128) Bde g_bdl[32];
uint16_t g_corb_n, g_rirb_n, g_rirb_rp;

volatile uint32_t g_irqs, g_bcis;

uint8_t r8(uint32_t o) { return mmio_read<uint8_t>(g_base + o); }
uint16_t r16(uint32_t o) { return mmio_read<uint16_t>(g_base + o); }
uint32_t r32(uint32_t o) { return mmio_read<uint32_t>(g_base + o); }
void w8(uint32_t o, uint8_t v) { mmio_write<uint8_t>(g_base + o, v); }
void w16(uint32_t o, uint16_t v) { mmio_write<uint16_t>(g_base + o, v); }
void w32(uint32_t o, uint32_t v) { mmio_write<uint32_t>(g_base + o, v); }

bool wait_bits(uint32_t off, uint32_t mask, uint32_t want, uint32_t ms, int width)
{
    const uint64_t end = clock::ms() + ms;
    for (;;) {
        const uint32_t v = width == 8 ? r8(off) : width == 16 ? r16(off) : r32(off);
        if ((v & mask) == want) return true;
        if (clock::ms() >= end) return false;
    }
}

uint16_t ring_size(uint32_t size_reg, uint16_t& n)     // pick the largest size the controller offers
{
    const uint8_t cap = r8(size_reg) >> 4;               // bit 4: 2 entries, 5: 16, 6: 256
    if (cap & 4) { n = 256; return 2; }
    if (cap & 2) { n = 16; return 1; }
    n = 2;
    return 0;
}

uint32_t sd(uint32_t index) { return hdareg::SD_BASE + index * hdareg::SD_SIZE; }

const char* const kWidget[] = {"audio output", "audio input", "mixer", "selector", "pin complex",
                               "power widget", "volume knob", "beep generator"};
const char* const kDevice[] = {"line out", "speaker", "HP out", "CD", "SPDIF out", "digital other out",
                               "modem line", "modem handset", "line in", "aux", "mic in", "telephony",
                               "SPDIF in", "digital other in", "reserved", "other"};
}  // namespace

namespace hda {
uint16_t format(uint32_t rate, uint8_t bits, uint8_t channels)
{
    // rate = base (48000 or 44100) * mult / div
    uint16_t base = 0, mult = 0, div = 0;
    switch (rate) {
    case 48000: break;
    case 44100: base = 1; break;
    case 96000: mult = 1; break;
    case 22050: base = 1; div = 1; break;
    case 24000: div = 1; break;
    case 16000: div = 2; break;
    case 8000: div = 5; break;
    default: break;
    }
#ifdef F412_RATE_BUG
    // The forensic build: a format table copied from a 48 kHz-only driver.
    if (rate == 44100) base = 0;
#endif
    uint16_t b = 1;
    if (bits == 8) b = 0;
    else if (bits == 20) b = 2;
    else if (bits == 24) b = 3;
    else if (bits == 32) b = 4;
    return static_cast<uint16_t>((base << 14) | (mult << 11) | (div << 8) | (b << 4) | ((channels - 1) & 0xF));
}

void describe_format(uint16_t f, char* out, uint32_t cap)
{
    const uint32_t base = (f & (1u << 14)) ? 44100 : 48000;
    const uint32_t mult = ((f >> 11) & 7) + 1, div = ((f >> 8) & 7) + 1;
    static const uint8_t bits[] = {8, 16, 20, 24, 32, 0, 0, 0};
    const uint32_t rate = base * mult / div;
    // tiny formatter: "<rate> Hz, <bits>-bit, <n> ch"
    char tmp[48];
    uint32_t n = 0;
    auto num = [&](uint32_t v) {
        char d[12];
        int k = 0;
        do { d[k++] = static_cast<char>('0' + v % 10); v /= 10; } while (v);
        while (k) tmp[n++] = d[--k];
    };
    auto str = [&](const char* s) { while (*s) tmp[n++] = *s++; };
    num(rate); str(" Hz, "); num(bits[(f >> 4) & 7]); str("-bit, "); num((f & 0xF) + 1u); str(" ch");
    tmp[n] = 0;
    for (uint32_t i = 0; i < cap; ++i) { out[i] = tmp[i]; if (!tmp[i]) break; }
    out[cap - 1] = 0;
}

bool find(PciAddr& a)
{
    static PciAddr found;
    static bool ok;
    ok = false;
    pci::enumerate([](PciAddr x) {
        if (!ok && pci::read16(x, pcireg::VENDOR_ID) == 0x8086 && (pci::read32(x, pcireg::CLASS_REVISION) >> 16) == 0x0403) {
            found = x;
            ok = true;
        }
    });
    a = found;
    return ok;
}

uint16_t codecs() { return g_codecs; }

bool init(PciAddr a, uint8_t vector)
{
    g_dev = a;
    Bar bars[6];
    pci::size_bars(a, bars, 6);
    g_base = static_cast<uintptr_t>(bars[0].addr);
    pci::enable(a, pcireg::CMD_MEMORY | pcireg::CMD_MASTER);
    const uint16_t gcap = r16(hdareg::GCAP);
    g_iss = (gcap >> 8) & 0xF;
    g_oss = (gcap >> 12) & 0xF;
    kprintf("hda: %02x:%02x.%x BAR0 0x%x, version %u.%u, GCAP 0x%04x: %u input, %u output, %u bidirectional streams, 64-bit %s\n",
            a.bus, a.dev, a.fn, static_cast<uint32_t>(g_base), r8(hdareg::VMAJ), r8(hdareg::VMIN), gcap, g_iss, g_oss,
            (gcap >> 3) & 0x1F, (gcap & 1) ? "yes" : "no");

    // Controller reset: CRST to 0 and back to 1, then give the codecs time to announce
    // themselves in STATESTS (the specification asks for at least 521 microseconds).
    w32(hdareg::GCTL, r32(hdareg::GCTL) & ~hdareg::GCTL_CRST);
    if (!wait_bits(hdareg::GCTL, hdareg::GCTL_CRST, 0, 100, 32)) return false;
    w32(hdareg::GCTL, r32(hdareg::GCTL) | hdareg::GCTL_CRST);
    if (!wait_bits(hdareg::GCTL, hdareg::GCTL_CRST, hdareg::GCTL_CRST, 100, 32)) return false;
    clock::sleep_ms(2);
    g_codecs = r16(hdareg::STATESTS);
    w16(hdareg::STATESTS, g_codecs);                      // write 1 to clear
    kprintf("hda: reset done, STATESTS 0x%04x (one bit per codec address)\n", g_codecs);

    // CORB and RIRB.
    w8(hdareg::CORBCTL, 0);
    w8(hdareg::RIRBCTL, 0);
    wait_bits(hdareg::CORBCTL, hdareg::CORBCTL_RUN, 0, 10, 8);
    wait_bits(hdareg::RIRBCTL, hdareg::RIRBCTL_RUN, 0, 10, 8);
    w8(hdareg::CORBSIZE, static_cast<uint8_t>(ring_size(hdareg::CORBSIZE, g_corb_n)));
    w8(hdareg::RIRBSIZE, static_cast<uint8_t>(ring_size(hdareg::RIRBSIZE, g_rirb_n)));
    const uint64_t corb = dma::map(a, g_corb, sizeof g_corb, false);
    const uint64_t rirb = dma::map(a, g_rirb, sizeof g_rirb, true);
    w32(hdareg::CORBLBASE, static_cast<uint32_t>(corb));
    w32(hdareg::CORBUBASE, static_cast<uint32_t>(corb >> 32));
    w32(hdareg::RIRBLBASE, static_cast<uint32_t>(rirb));
    w32(hdareg::RIRBUBASE, static_cast<uint32_t>(rirb >> 32));
    w16(hdareg::CORBWP, 0);
    w16(hdareg::CORBRP, hdareg::CORBRP_RST);              // reset the read pointer...
    wait_bits(hdareg::CORBRP, hdareg::CORBRP_RST, hdareg::CORBRP_RST, 10, 16);
    w16(hdareg::CORBRP, 0);                               // ...and take it out of reset
    wait_bits(hdareg::CORBRP, hdareg::CORBRP_RST, 0, 10, 16);
    w16(hdareg::RIRBWP, hdareg::RIRBWP_RST);
    g_rirb_rp = 0;
    w16(hdareg::RINTCNT, 0xFF);                           // response-interrupt count: 255
    w8(hdareg::RIRBCTL, hdareg::RIRBCTL_RUN);
    w8(hdareg::CORBCTL, hdareg::CORBCTL_RUN);
    kprintf("hda: CORB %u entries at 0x%x, RIRB %u entries at 0x%x, both running\n", g_corb_n,
            static_cast<uint32_t>(corb), g_rirb_n, static_cast<uint32_t>(rirb));

    g_use_irq = vector != 0;
    if (g_use_irq) w32(hdareg::INTCTL, hdareg::INTCTL_GIE);
    return g_codecs != 0;
}

uint32_t command(uint8_t cad, uint8_t nid, uint32_t verb20)
{
    const uint32_t word = (uint32_t{cad} << 28) | (uint32_t{nid} << 20) | (verb20 & 0xFFFFF);
    const uint16_t wp = static_cast<uint16_t>((r16(hdareg::CORBWP) + 1) % g_corb_n);
    g_corb[wp] = word;
    compiler_barrier();
    w16(hdareg::CORBWP, wp);                              // the controller fetches up to WP
    const uint16_t want = static_cast<uint16_t>((g_rirb_rp + 1) % g_rirb_n);
    const uint64_t end = clock::ms() + 100;
    while ((r16(hdareg::RIRBWP) & 0xFF) != (want & 0xFF)) {
        if (clock::ms() >= end) {
            kprintf("hda: no response to verb 0x%08x (RIRBWP %u)\n", word, r16(hdareg::RIRBWP));
            return 0xFFFFFFFFu;
        }
    }
    compiler_barrier();
    g_rirb_rp = want;
    w8(hdareg::RIRBSTS, 0x05);                            // clear RINTFL and RIRBOIS (write 1)
    return g_rirb[want * 2];                              // [want*2 + 1] = codec address etc.
}

uint32_t param(uint8_t cad, uint8_t nid, uint8_t id) { return command(cad, nid, (verb::GET_PARAM << 8) | id); }

bool walk(uint8_t cad, Path& out)
{
    const uint32_t vid = param(cad, 0, verb::P_VENDOR), rev = param(cad, 0, verb::P_REVISION);
    const uint32_t root = param(cad, 0, verb::P_NODES);
    if (vid == 0xFFFFFFFFu || root == 0xFFFFFFFFu) return false;
    kprintf("codec %u: vendor 0x%04x device 0x%04x, revision 0x%08x; root node: %u function group(s) from node %u\n", cad,
            vid >> 16, vid & 0xFFFF, rev, root & 0xFF, (root >> 16) & 0xFF);
    out = Path{cad, 0, 0, 0};
    for (uint32_t fg = (root >> 16) & 0xFF; fg < ((root >> 16) & 0xFF) + (root & 0xFF); ++fg) {
        const uint32_t type = param(cad, static_cast<uint8_t>(fg), verb::P_FG_TYPE) & 0xFF;
        if (type != 1) { kprintf("  node %u: function group type %u (not audio), skipped\n", fg, type); continue; }
        out.afg = static_cast<uint8_t>(fg);
        command(cad, out.afg, (verb::SET_POWER << 8) | 0);   // D0: fully on
        const uint32_t sub = param(cad, out.afg, verb::P_NODES);
        if (sub == 0xFFFFFFFFu) return false;
        const uint32_t pcm = param(cad, out.afg, verb::P_PCM);
        kprintf("  node %u: audio function group, widgets %u..%u, PCM rates/sizes 0x%08x, formats 0x%x\n", fg,
                (sub >> 16) & 0xFF, ((sub >> 16) & 0xFF) + (sub & 0xFF) - 1, pcm, param(cad, out.afg, verb::P_FORMATS));
        for (uint32_t nid = (sub >> 16) & 0xFF; nid < ((sub >> 16) & 0xFF) + (sub & 0xFF); ++nid) {
            const uint8_t n = static_cast<uint8_t>(nid);
            const uint32_t wcap = param(cad, n, verb::P_WCAP);
            const uint32_t wtype = (wcap >> 20) & 0xF;
            kprintf("    widget %u: %s, caps 0x%08x%s%s", nid, wtype < 8 ? kWidget[wtype] : "vendor", wcap,
                    (wcap & 1) ? ", stereo" : "", (wcap & 4) ? ", output amp" : "");
            if (wcap & (1u << 8)) {                         // has a connection list
                const uint32_t len = param(cad, n, verb::P_CONN_LEN) & 0x7F;
                const uint32_t list = command(cad, n, (verb::GET_CONN_LIST << 8) | 0);
                kprintf(", inputs from:");
                for (uint32_t i = 0; i < len && i < 4; ++i) kprintf(" %u", (list >> (8 * i)) & 0xFF);
                if (wtype == 4 && !out.pin) {
                    const uint32_t pincap = param(cad, n, verb::P_PIN_CAP);
                    if ((pincap & (1u << 4)) && out.dac && len && (list & 0xFF) == out.dac) out.pin = n;
                }
            }
            if (wtype == 4) {
                const uint32_t cfg = command(cad, n, verb::GET_CONFIG_DEFAULT << 8);
                kprintf(", pin caps 0x%08x, default device: %s", param(cad, n, verb::P_PIN_CAP), kDevice[(cfg >> 20) & 0xF]);
            }
            if (wtype == 0 && !out.dac) out.dac = n;
            kprintf("\n");
        }
    }
    if (!out.dac || !out.pin) return false;
    kprintf("  path: converter (DAC) %u -> pin %u\n", out.dac, out.pin);
    return true;
}

void irq(uint8_t)
{
    g_irqs = g_irqs + 1;
    const uint32_t s = sd(g_iss);                         // the first output stream
    const uint8_t sts = r8(s + hdareg::SD_STS);
    if (sts & hdareg::SDSTS_BCIS) g_bcis = g_bcis + 1;
    w8(s + hdareg::SD_STS, sts);                          // write 1 to clear what we saw
}

PlayStats play(const Path& p, const void* buf, uint32_t bytes, uint16_t fmt, uint32_t periods, uint32_t stop_after)
{
    PlayStats st{0, 0, 0, 0, 0};
    const uint32_t s = sd(g_iss);                         // output stream descriptor 0
    const uint8_t tag = 1;                                // stream number on the link (1..15)
    auto ctl_read = [&] { return r32(s + hdareg::SD_CTL) & 0x00FFFFFFu; };
    auto ctl_write = [&](uint32_t v) { w32(s + hdareg::SD_CTL, v & 0x00FFFFFFu); };

    // Stream reset: SRST to 1, wait, back to 0, wait.
    ctl_write(ctl_read() | hdareg::SDCTL_SRST);
    wait_bits(s + hdareg::SD_CTL, hdareg::SDCTL_SRST, hdareg::SDCTL_SRST, 10, 8);
    ctl_write(ctl_read() & ~hdareg::SDCTL_SRST);
    wait_bits(s + hdareg::SD_CTL, hdareg::SDCTL_SRST, 0, 10, 8);

    // Buffer descriptor list: 'periods' equal parts, an interrupt at the end of each.
    const uint64_t iova = dma::map(g_dev, buf, bytes, false);
    const uint32_t part = bytes / periods;
    for (uint32_t i = 0; i < periods; ++i) {
        const uint64_t a = iova + uint64_t{i} * part;
        g_bdl[i] = Bde{static_cast<uint32_t>(a), static_cast<uint32_t>(a >> 32), part, 1};
    }
    const uint64_t bdl = dma::map(g_dev, g_bdl, sizeof g_bdl, false);
    w32(s + hdareg::SD_BDPL, static_cast<uint32_t>(bdl));
    w32(s + hdareg::SD_BDPU, static_cast<uint32_t>(bdl >> 32));
    w32(s + hdareg::SD_CBL, part * periods);
    w16(s + hdareg::SD_LVI, static_cast<uint16_t>(periods - 1));
    w16(s + hdareg::SD_FMT, fmt);
    // SDnCTL is 3 bytes and SDnSTS the 4th. A 32-bit write reaches both, so the top byte is
    // written as 0: status bits are write-1-to-clear and 0 leaves them alone. (A byte write
    // to offset 2 would be cleaner, but QEMU's model ignores it: see the lab notes.)
    ctl_write((ctl_read() & 0x000FFFFFu) | (uint32_t{tag} << hdareg::SDCTL_STRM_SHIFT));

    // The codec side: converter listens to stream 'tag' channel 0 with the same format;
    // amplifiers unmuted at 0 dB; the pin drives its output.
    command(p.cad, p.dac, (verb::SET_STREAM_ID << 8) | (uint32_t{tag} << 4) | 0);
    command(p.cad, p.dac, (verb::SET_FORMAT4 << 16) | fmt);
    const uint32_t amp = param(p.cad, p.dac, verb::P_OUT_AMP);
    const uint32_t gain0 = amp & 0x7F;                    // the step that means 0 dB
    command(p.cad, p.dac, (verb::SET_AMP4 << 16) | verb::AMP_OUT | verb::AMP_LEFT | verb::AMP_RIGHT | gain0);
    if (param(p.cad, p.pin, verb::P_WCAP) & (1u << 2))  // only widgets that HAVE an output amp
        command(p.cad, p.pin, (verb::SET_AMP4 << 16) | verb::AMP_OUT | verb::AMP_LEFT | verb::AMP_RIGHT |
                                  (param(p.cad, p.pin, verb::P_OUT_AMP) & 0x7F));
    command(p.cad, p.pin, (verb::SET_PIN_CTL << 8) | verb::PIN_OUT_EN);
    const uint32_t back = command(p.cad, p.dac, verb::GET_FORMAT4 << 16);
    const uint32_t gl = command(p.cad, p.dac, (verb::GET_AMP4 << 16) | verb::AMP_OUT | verb::AMP_LEFT);
    const uint32_t gr = command(p.cad, p.dac, (verb::GET_AMP4 << 16) | verb::AMP_OUT);
    char d1[48], d2[48];
    describe_format(fmt, d1, sizeof d1);
    describe_format(static_cast<uint16_t>(back), d2, sizeof d2);
    kprintf("stream: SD%u (first output), tag %u, %u periods of %u bytes, BDL at 0x%x, format 0x%04x = %s; "
            "converter reads back 0x%04x = %s; DAC amp cap 0x%08x -> gain step %u, reads back left 0x%02x right 0x%02x\n",
            g_iss, tag, periods, part, static_cast<uint32_t>(bdl), fmt, d1, back & 0xFFFF, d2, amp, gain0, gl, gr);

    // Go. Interrupt on completion of each buffer entry (IOCE), stream bit in INTCTL.
    g_irqs = 0;
    g_bcis = 0;
    w8(s + hdareg::SD_STS, hdareg::SDSTS_BCIS | hdareg::SDSTS_FIFOE | hdareg::SDSTS_DESE);
    if (g_use_irq) w32(hdareg::INTCTL, hdareg::INTCTL_GIE | (1u << g_iss));
    const uint64_t t0 = clock::ms();
    ctl_write(ctl_read() | hdareg::SDCTL_RUN | hdareg::SDCTL_IOCE);
    const uint64_t end = t0 + 3000 + uint64_t{stop_after} * 2000;
    while (g_bcis < stop_after && clock::ms() < end) {
        if (g_use_irq) {
            intr::wait();
        } else if (r8(s + hdareg::SD_STS) & hdareg::SDSTS_BCIS) {
            w8(s + hdareg::SD_STS, hdareg::SDSTS_BCIS);
            g_bcis = g_bcis + 1;
        }
    }
    st.ms = clock::ms() - t0;
    ctl_write(ctl_read() & ~(hdareg::SDCTL_RUN | hdareg::SDCTL_IOCE));
    wait_bits(s + hdareg::SD_CTL, hdareg::SDCTL_RUN, 0, 10, 8);
    st.periods = g_bcis;
    st.irqs = g_irqs;
    st.lpib_last = r32(s + hdareg::SD_LPIB);
    st.fifoe = (r8(s + hdareg::SD_STS) & hdareg::SDSTS_FIFOE) ? 1 : 0;
    w32(hdareg::INTCTL, g_use_irq ? hdareg::INTCTL_GIE : 0);
    return st;
}
}  // namespace hda
