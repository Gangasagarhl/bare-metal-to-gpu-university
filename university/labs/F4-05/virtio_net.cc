// virtio_net.cc - DR301 F4-05: virtio network device. Queue 0 receives, queue 1 sends.
// Every packet is preceded by a 12-byte virtio-net header (all zero here: no checksum
// offload, no segmentation offload were negotiated).
#include "virtio_net.h"
#include "kbase.h"
#include "driver.h"
#include "virtio.h"

namespace {
constexpr unsigned F_MAC = 5;                    // the device config holds a MAC address
constexpr size_t HDR = 12, BUF = 2048, NRX = 16;
vio::Device g_dev;
vio::Queue g_rx, g_tx;
alignas(4096) uint8_t g_rxbuf[NRX][BUF];
alignas(4096) uint8_t g_txbuf[BUF];
uint8_t g_mac[6];

void post_rx(int i)
{
    const int h = vio::alloc_chain(g_rx, 1);
    g_rx.desc[h].addr = reinterpret_cast<uintptr_t>(g_rxbuf[i]);
    g_rx.desc[h].len = BUF;
    g_rx.desc[h].flags = vio::DESC_WRITE;
    vio::submit(g_rx, static_cast<uint16_t>(h));
}
}  // namespace

namespace vnet {
int init(PciAddr a)
{
    int rc = vio::init(g_dev, a, uint64_t{1} << F_MAC);
    if (rc) return rc;
    if ((rc = vio::setup_queue(g_dev, g_rx, 0)) || (rc = vio::setup_queue(g_dev, g_tx, 1))) return rc;
    for (int i = 0; i < 6; ++i) g_mac[i] = g_dev.device_cfg[i];
    for (size_t i = 0; i < NRX; ++i) post_rx(static_cast<int>(i));
    vio::driver_ok(g_dev);
    vio::kick(g_rx);                             // tell the device the receive buffers exist
    kprintf("virtio-net: MAC %02x:%02x:%02x:%02x:%02x:%02x, %u receive buffers posted\n", g_mac[0],
            g_mac[1], g_mac[2], g_mac[3], g_mac[4], g_mac[5], static_cast<uint32_t>(NRX));
    return 0;
}

const uint8_t* mac() { return g_mac; }

void send(const uint8_t* frame, size_t len)
{
    memset(g_txbuf, 0, HDR);
    memcpy(g_txbuf + HDR, frame, len);
    const int h = vio::alloc_chain(g_tx, 1);
    g_tx.desc[h].addr = reinterpret_cast<uintptr_t>(g_txbuf);
    g_tx.desc[h].len = static_cast<uint32_t>(HDR + len);
    g_tx.desc[h].flags = 0;                      // device reads it
    vio::submit(g_tx, static_cast<uint16_t>(h));
    vio::kick(g_tx);
    uint32_t head, l;
    while (!vio::get_used(g_tx, head, l)) { }
    vio::free_chain(g_tx, static_cast<uint16_t>(head));
}

size_t receive(uint8_t* frame, size_t cap, uint32_t spins)
{
    uint32_t head, len;
    while (!vio::get_used(g_rx, head, len))
        if (spins-- == 0) return 0;
    const uintptr_t addr = static_cast<uintptr_t>(g_rx.desc[head].addr);
    const int i = static_cast<int>((addr - reinterpret_cast<uintptr_t>(g_rxbuf[0])) / BUF);
    size_t n = len > HDR ? len - HDR : 0;
    if (n > cap) n = cap;
    memcpy(frame, g_rxbuf[i] + HDR, n);
    vio::free_chain(g_rx, static_cast<uint16_t>(head));
    post_rx(i);                                  // give the buffer back to the device
    vio::kick(g_rx);
    return n;
}
}  // namespace vnet
