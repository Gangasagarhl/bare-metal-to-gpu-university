// e1000.cc - DR302 F4-10: 82540EM reset, MAC address, receive and transmit rings (polled).
#include "e1000.h"
#include "../F4-01/kbase.h"
#include "../F4-08/dma.h"
#include "../F4-08/intr.h"

using namespace e1000reg;

namespace {
constexpr int NRX = 32, NTX = 32;
constexpr uint16_t BUF = 2048;
PciAddr g_pci{0xFF, 0, 0}, g_found{0xFF, 0, 0};
uintptr_t g_mmio = 0;
uint8_t g_mac[6];
alignas(128) e1000::RxDesc g_rx[NRX];
alignas(128) e1000::TxDesc g_tx[NTX];
alignas(4096) uint8_t g_rxbuf[NRX][BUF];
alignas(4096) uint8_t g_txbuf[NTX][BUF];
uint32_t g_rx_next = 0, g_tx_next = 0;

void probe(PciAddr a)
{
    if (pci::read16(a, pcireg::VENDOR_ID) == 0x8086 && pci::read16(a, pcireg::DEVICE_ID) == 0x100E) g_found = a;
}
}  // namespace

namespace e1000 {
uint32_t read(uint32_t reg) { return mmio_read<uint32_t>(g_mmio + reg); }
void write(uint32_t reg, uint32_t v) { mmio_write<uint32_t>(g_mmio + reg, v); }
const uint8_t* mac() { return g_mac; }
bool link_up() { return read(STATUS) & STATUS_LU; }

bool find(PciAddr& out)
{
    pci::enumerate(probe);
    out = g_found;
    return g_found.bus != 0xFF;
}

uint16_t eeprom_word(uint8_t addr)
{
    write(EERD, (uint32_t{addr} << 8) | EERD_START);
    for (int guard = 0; guard < 100000; ++guard) {
        const uint32_t v = read(EERD);
        if (v & EERD_DONE) return static_cast<uint16_t>(v >> 16);
    }
    return 0xFFFF;
}

bool init(PciAddr a)
{
    g_pci = a;
    Bar bars[6];
    pci::size_bars(a, bars, 6);
    g_mmio = static_cast<uintptr_t>(bars[0].addr);
    pci::enable(a, pcireg::CMD_MEMORY | pcireg::CMD_MASTER);
    dma::attach(a);

    write(IMC, 0xFFFFFFFFu);                          // no interrupts: this driver polls
    write(CTRL, read(CTRL) | CTRL_RST);               // global reset; the bit clears itself
    clock::sleep_ms(2);
    for (int guard = 0; read(CTRL) & CTRL_RST; ++guard)
        if (guard > 100000) return false;
    write(IMC, 0xFFFFFFFFu);
    write(CTRL, read(CTRL) | CTRL_SLU | CTRL_ASDE);   // set link up, auto speed detection

    // MAC address: three 16-bit words at the start of the EEPROM, low byte first.
    for (uint8_t i = 0; i < 3; ++i) {
        const uint16_t w = eeprom_word(i);
        g_mac[2 * i] = w & 0xFF;
        g_mac[2 * i + 1] = w >> 8;
    }
    write(RAL0, g_mac[0] | g_mac[1] << 8 | g_mac[2] << 16 | uint32_t{g_mac[3]} << 24);
    write(RAH0, g_mac[4] | g_mac[5] << 8 | (1u << 31));   // AV: address valid
    for (int i = 0; i < 128; ++i) write(MTA + 4 * i, 0);  // no multicast

    for (int i = 0; i < NRX; ++i) {
        g_rx[i] = RxDesc{dma::map(a, g_rxbuf[i], BUF, true), 0, 0, 0, 0, 0};
    }
    const uint64_t rx = dma::map(a, g_rx, sizeof g_rx, true);
    write(RDBAL, static_cast<uint32_t>(rx));
    write(RDBAH, static_cast<uint32_t>(rx >> 32));
    write(RDLEN, sizeof g_rx);
    write(RDH, 0);
    write(RDT, NRX - 1);                              // all but one descriptor owned by the NIC
    write(RCTL, RCTL_EN | RCTL_BAM | RCTL_SECRC);     // 2048-byte buffers, broadcast, strip CRC

    for (int i = 0; i < NTX; ++i) g_tx[i] = TxDesc{dma::map(a, g_txbuf[i], BUF, false), 0, 0, 0, TXD_DD, 0, 0};
    const uint64_t tx = dma::map(a, g_tx, sizeof g_tx, true);
    write(TDBAL, static_cast<uint32_t>(tx));
    write(TDBAH, static_cast<uint32_t>(tx >> 32));
    write(TDLEN, sizeof g_tx);
    write(TDH, 0);
    write(TDT, 0);
    write(TCTL, TCTL_EN | TCTL_PSP | (0x10u << 4) | (0x40u << 12));   // collision threshold, distance
    write(TIPG, 10u | (8u << 10) | (6u << 20));                        // inter-packet gap
    kprintf("e1000: %02x:%02x.%x MAC %02x:%02x:%02x:%02x:%02x:%02x, link %s, %d rx / %d tx descriptors\n",
            a.bus, a.dev, a.fn, g_mac[0], g_mac[1], g_mac[2], g_mac[3], g_mac[4], g_mac[5],
            link_up() ? "up" : "down", NRX, NTX);
    return true;
}

bool send(const void* frame, uint16_t len)
{
    if (len > BUF) return false;
    TxDesc& d = g_tx[g_tx_next];
    for (int guard = 0; !(d.status & TXD_DD); ++guard) {   // previous use of this slot done?
        if (guard > 1000000) return false;
        compiler_barrier();
    }
    memcpy(g_txbuf[g_tx_next], frame, len);
    d.length = len < 60 ? 60 : len;                   // pad short frames to the Ethernet minimum
    if (len < 60) memset(g_txbuf[g_tx_next] + len, 0, 60 - len);
    d.cmd = TXD_EOP | TXD_IFCS | TXD_RS;
    d.status = 0;
    compiler_barrier();
    g_tx_next = (g_tx_next + 1) % NTX;
    write(TDT, g_tx_next);                            // hand the descriptor to the NIC
    return true;
}

uint16_t receive(void* buf, uint16_t max)
{
    RxDesc& d = g_rx[g_rx_next];
    compiler_barrier();
    if (!(d.status & RXD_DD)) return 0;
    uint16_t len = d.length;
    if (!(d.status & RXD_EOP) || d.errors) len = 0;   // drop multi-buffer or bad frames
    if (len > max) len = 0;
    if (len) memcpy(buf, g_rxbuf[g_rx_next], len);
    d.status = 0;
    compiler_barrier();
    write(RDT, g_rx_next);                            // give the buffer back
    g_rx_next = (g_rx_next + 1) % NRX;
    return len;
}
}  // namespace e1000
