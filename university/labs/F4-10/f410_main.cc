// f410_main.cc - DR302 F4-10: milestone C9 in QEMU with user-mode networking.
// DHCP, ping, a TCP echo server for a host client, an HTTP fetch from a host-side server,
// then the same transfers with 5 % induced segment loss in both directions.
// -DF410_TIMER_BUG builds the forensic lab's "nightly upload" kernel instead.
#include "../F4-01/kbase.h"
#include "../F4-02/acpi.h"
#include "../F4-08/intr.h"
#include "../F4-08/dma.h"
#include "../F4-08/vtd.h"
#include "net.h"
#include "tcp.h"
#include "e1000.h"

namespace {
using net::ip;
uint8_t g_buf[65536];
uint8_t g_body[300000];

// HTTP/1.0 GET over our TCP; returns the body length (or -1) and its fnv1a.
[[maybe_unused]] int http_get(const char* path, uint32_t& hash, bool show)
{
    tcp::Conn* c = tcp::connect(ip(10, 0, 2, 100), 80, 20000);
    if (!c) { kprintf("http: connect failed\n"); return -1; }
    char req[128];
    const char* parts[] = {"GET ", path, " HTTP/1.0\r\nHost: 10.0.2.100\r\nUser-Agent: dr302-f4-10\r\n\r\n"};
    uint32_t n = 0;
    for (const char* p : parts) for (; *p; ++p) req[n++] = *p;
    tcp::send(c, req, n, 5000);
    uint32_t got = 0;
    for (;;) {
        const int r = tcp::recv(c, g_body + got, sizeof g_body - got, 30000);
        if (r <= 0) { if (r < 0) kprintf("http: receive failed after %u bytes\n", got); break; }
        got += static_cast<uint32_t>(r);
    }
    tcp::close(c, 5000);
    uint32_t hdr = 0;                                  // the body starts after the blank line
    while (hdr + 3 < got && memcmp(g_body + hdr, "\r\n\r\n", 4) != 0) ++hdr;
    if (hdr + 3 >= got) return -1;
    g_body[got < sizeof g_body ? got : sizeof g_body - 1] = 0;
    uint32_t eol = 0;
    while (eol < hdr && g_body[eol] != '\r') ++eol;
    char status[64] = {};
    memcpy(status, g_body, eol < 63 ? eol : 63);
    const uint32_t body = hdr + 4, blen = got - body;
    hash = fnv1a(g_body + body, blen);
    kprintf("http: GET %s -> \"%s\", %u header bytes, %u body bytes, body fnv1a 0x%08x\n", path, status, body,
            blen, hash);
    if (show) kprintf("http: body begins: %s", reinterpret_cast<char*>(g_body + body));
    tcp::print(c, "http: tcp");
    return static_cast<int>(blen);
}

// Sends 'len' pattern bytes to the host's sink (guest port 10.0.2.100:9) and prints its reply.
bool upload(uint32_t len, uint32_t timeout_ms)
{
    tcp::Conn* c = tcp::connect(ip(10, 0, 2, 100), 9, 20000);
    if (!c) { kprintf("upload: connect failed\n"); return false; }
    uint32_t h = 2166136261u, sent = 0;
    const uint64_t start = clock::ms();
    while (sent < len) {
        const uint32_t n = len - sent < sizeof g_buf ? len - sent : sizeof g_buf;
        for (uint32_t i = 0; i < n; ++i) g_buf[i] = static_cast<uint8_t>((sent + i) * 31 + ((sent + i) >> 11));
        h = fnv1a(g_buf, n, h);
        const uint32_t left = static_cast<uint32_t>(start + timeout_ms > clock::ms() ? start + timeout_ms - clock::ms() : 0);
        if (tcp::send(c, g_buf, n, left) != static_cast<int>(n)) { kprintf("upload: send timed out\n"); break; }
        sent += n;
    }
    kprintf("upload: %u bytes queued (fnv1a 0x%08x), closing\n", sent, h);
    const uint64_t left = start + timeout_ms > clock::ms() ? start + timeout_ms - clock::ms() : 1;
    tcp::Conn* cc = c;
    // Half-close: our FIN tells the sink the data is complete; its reply follows.
    bool ok = tcp::close(cc, static_cast<uint32_t>(left)) || tcp::state(cc) == tcp::State::FinWait2;
    char reply[96] = {};
    const int r = ok ? tcp::recv(cc, reply, sizeof reply - 1, 5000) : -1;
    if (r > 0) kprintf("upload: sink replied: %s", reply);
    else kprintf("upload: no reply from the sink (state %s after %u ms)\n", tcp::state_name(tcp::state(cc)),
                 static_cast<uint32_t>(clock::ms() - start));
    tcp::print(cc, "upload: tcp");
    return r > 0;
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-10 kernel: magic=0x%x\n", magic);
    intr::init();
    uint64_t ecam;
    uint8_t b0, b1;
    if (acpi::ecam(ecam, b0, b1)) pci::use_ecam(ecam, b0, b1);
    const bool iommu = dma::use_iommu();
    if (!iommu) dma::use_identity();
    if (!net::init()) panic("no e1000");
    if (iommu) vtd::enable();
    if (!net::dhcp(3000)) panic("DHCP failed");
    for (uint16_t s = 1; s <= 3; ++s) {
        const int ms = net::ping(net::cfg.gateway, s, 1000);
        net::print_ip("icmp: echo reply from ", net::cfg.gateway);
        kprintf(" seq %u: %d ms\n", s, ms);
    }
    bool ok = true;
#ifdef F410_TIMER_BUG
    // The nightly job: upload 512 KiB over a link that loses 5 % of segments.
    net::set_loss(5, 0, 0x6E1A0005u);
    tcp::set_trace(true);
    kprintf("nightly: uploading 524288 bytes, induced loss 5 %% (transmit side)\n");
    ok = upload(524288, 20000);
#else
    // 1. TCP echo server: the host client connects through QEMU's host forwarding.
    kprintf("echo: listening on port 7\n");
    tcp::Conn* e = tcp::accept(7, 30000);
    if (!e) panic("echo: nobody connected");
    uint32_t echoed = 0;
    for (;;) {
        const int r = tcp::recv(e, g_buf, sizeof g_buf, 30000);
        if (r <= 0) break;
        tcp::send(e, g_buf, static_cast<uint32_t>(r), 30000);
        echoed += static_cast<uint32_t>(r);
    }
    tcp::close(e, 5000);
    kprintf("echo: connection closed after %u bytes echoed\n", echoed);
    tcp::print(e, "echo: tcp");

    // 2. The C9 project: fetch a web page from the host-side server.
    uint32_t h1 = 0, h2 = 0;
    ok = http_get("/index.html", h1, true) > 0;
    ok = http_get("/big.bin", h2, false) == 262144 && ok;

    // 3. The same with 5 % of TCP segments lost in each direction (induced in this kernel).
    net::set_loss(5, 5, 0x0C0FFEE5u);
    kprintf("loss: 5 %% of TCP segments dropped on transmit and on receive from now on\n");
    uint32_t h3 = 0;
    ok = http_get("/big.bin", h3, false) == 262144 && h3 == h2 && ok;
    kprintf("loss: big.bin under loss %s the loss-free copy\n", h3 == h2 ? "matches" : "DIFFERS from");
    ok = upload(262144, 60000) && ok;
    net::set_loss(0, 0, 0);
#endif
    kprintf("net: frames rx %u tx %u, induced drops tx %u rx %u, bad checksums %u, DMA %s\n", net::stats.rx_frames,
            net::stats.tx_frames, net::stats.tx_dropped_loss, net::stats.rx_dropped_loss, net::stats.rx_bad_checksum,
            iommu ? "through VT-d" : "identity");
    // Frames the NIC itself could not deliver (receive ring full), from its statistics registers.
    const uint32_t mpc = e1000::read(e1000reg::MPC), rnbc = e1000::read(e1000reg::RNBC);
    kprintf("e1000: missed packets (MPC) %u, receive-no-buffers events (RNBC) %u\n", mpc, rnbc);
    kprintf("F4-10 %s\n", ok ? "done" : "FAILED");
    qemu_exit(ok ? 0x10 : 0x01);
}
