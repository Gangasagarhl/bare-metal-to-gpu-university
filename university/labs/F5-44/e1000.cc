// e1000.cc - DS403 cluster kernel: polled e1000 driver (receive and transmit rings).
// Register offsets and bits follow the Intel 8254x Software Developer's Manual (registers
// chapter); see the unverified box in F5-44: they were written from that manual's layout as
// recalled, then confirmed only by this driver working on QEMU's e1000 model.
#include "e1000.h"
#include "k.h"

namespace {
constexpr uint32_t CTRL = 0x0000, STATUS = 0x0008, EERD = 0x0014, IMC = 0x00D8;
constexpr uint32_t RCTL = 0x0100, TCTL = 0x0400, TIPG = 0x0410;
constexpr uint32_t RDBAL = 0x2800, RDBAH = 0x2804, RDLEN = 0x2808, RDH = 0x2810, RDT = 0x2818;
constexpr uint32_t TDBAL = 0x3800, TDBAH = 0x3804, TDLEN = 0x3808, TDH = 0x3810, TDT = 0x3818;
constexpr uint32_t MTA = 0x5200, RAL0 = 0x5400, RAH0 = 0x5404;

struct RxDesc { uint64_t addr; uint16_t length; uint16_t csum; uint8_t status; uint8_t errors; uint16_t special; };
struct TxDesc { uint64_t addr; uint16_t length; uint8_t cso; uint8_t cmd; uint8_t status; uint8_t css; uint16_t special; };
static_assert(sizeof(RxDesc) == 16 && sizeof(TxDesc) == 16, "descriptors are 16 bytes");

constexpr int NRX = 32, NTX = 16, BUF = 2048;
alignas(128) RxDesc rx[NRX];
alignas(128) TxDesc tx[NTX];
alignas(16) uint8_t rxbuf[NRX][BUF];
alignas(16) uint8_t txbuf[NTX][BUF];
int rx_next = 0, tx_next = 0;
volatile uint32_t* regs = nullptr;
uint8_t mac[6];

uint32_t rd(uint32_t off) { return regs[off / 4]; }
void wr(uint32_t off, uint32_t v) { regs[off / 4] = v; }

uint32_t pci_read(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off)
{
    outl(0xCF8, 0x80000000u | (bus << 16) | (dev << 11) | (fn << 8) | (off & 0xFC));
    return inl(0xCFC);
}

void pci_write(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off, uint32_t v)
{
    outl(0xCF8, 0x80000000u | (bus << 16) | (dev << 11) | (fn << 8) | (off & 0xFC));
    outl(0xCFC, v);
}

uint16_t eeprom_word(uint8_t addr)
{
    wr(EERD, (static_cast<uint32_t>(addr) << 8) | 1);   // start a read of one 16-bit word
    for (int i = 0; i < 100000; ++i) {
        uint32_t v = rd(EERD);
        if (v & (1u << 4)) return static_cast<uint16_t>(v >> 16);   // DONE bit
    }
    return 0xFFFF;
}
}  // namespace

bool e1000_init()
{
    // 1. Find vendor 0x8086 device 0x100E on bus 0 (QEMU's e1000).
    int found = -1;
    for (int dev = 0; dev < 32 && found < 0; ++dev)
        if (pci_read(0, static_cast<uint8_t>(dev), 0, 0x00) == 0x100E8086u) found = dev;
    if (found < 0) return false;
    uint8_t d = static_cast<uint8_t>(found);
    uint32_t bar0 = pci_read(0, d, 0, 0x10);
    regs = reinterpret_cast<volatile uint32_t*>(bar0 & ~0xFu);
    pci_write(0, d, 0, 0x04, pci_read(0, d, 0, 0x04) | 0x6);   // memory space + bus master

    // 2. Reset, mask every interrupt, set link up.
    wr(IMC, 0xFFFFFFFFu);
    wr(CTRL, rd(CTRL) | (1u << 26));                            // RST
    for (int i = 0; i < 100000 && (rd(CTRL) & (1u << 26)); ++i) { }
    wr(IMC, 0xFFFFFFFFu);
    wr(CTRL, rd(CTRL) | (1u << 6) | (1u << 5));                 // SLU, ASDE

    // 3. MAC address from the EEPROM words 0..2; program receive address 0 (AV = bit 31).
    for (int i = 0; i < 3; ++i) {
        uint16_t w = eeprom_word(static_cast<uint8_t>(i));
        mac[2 * i] = static_cast<uint8_t>(w & 0xFF);
        mac[2 * i + 1] = static_cast<uint8_t>(w >> 8);
    }
    wr(RAL0, mac[0] | (mac[1] << 8) | (mac[2] << 16) | (static_cast<uint32_t>(mac[3]) << 24));
    wr(RAH0, mac[4] | (mac[5] << 8) | (1u << 31));
    for (int i = 0; i < 128; ++i) wr(MTA + 4 * i, 0);

    // 4. Receive ring: every descriptor owns a 2048-byte buffer; tail = last descriptor.
    for (int i = 0; i < NRX; ++i) {
        rx[i] = RxDesc{reinterpret_cast<uint32_t>(&rxbuf[i][0]), 0, 0, 0, 0, 0};
    }
    wr(RDBAL, reinterpret_cast<uint32_t>(&rx[0]));
    wr(RDBAH, 0);
    wr(RDLEN, sizeof rx);
    wr(RDH, 0);
    wr(RDT, NRX - 1);
    // EN (bit 1), BAM accept broadcast (bit 15), SECRC strip CRC (bit 26); BSIZE 00 = 2048.
    wr(RCTL, (1u << 1) | (1u << 15) | (1u << 26));

    // 5. Transmit ring.
    for (int i = 0; i < NTX; ++i) {
        tx[i] = TxDesc{reinterpret_cast<uint32_t>(&txbuf[i][0]), 0, 0, 0, 1, 0, 0};   // DD set: free
    }
    wr(TDBAL, reinterpret_cast<uint32_t>(&tx[0]));
    wr(TDBAH, 0);
    wr(TDLEN, sizeof tx);
    wr(TDH, 0);
    wr(TDT, 0);
    // EN (bit 1), PSP pad short packets (bit 3), collision threshold and distance fields.
    wr(TCTL, (1u << 1) | (1u << 3) | (0x10u << 4) | (0x40u << 12));
    wr(TIPG, 0x0060200A);
    return (rd(STATUS) & (1u << 1)) != 0;                        // LU: link up
}

const uint8_t* e1000_mac() { return mac; }

bool e1000_send(const void* frame, uint16_t len)
{
    asm volatile("" : : : "memory");                              // re-read what the NIC wrote
    TxDesc& t = tx[tx_next];
    if ((t.status & 1) == 0 || len > BUF) return false;          // descriptor still in use
    memcpy(txbuf[tx_next], frame, len);
    t.length = len;
    t.cmd = (1u << 0) | (1u << 1) | (1u << 3);                    // EOP, IFCS, RS
    t.status = 0;
    tx_next = (tx_next + 1) % NTX;
    wr(TDT, static_cast<uint32_t>(tx_next));                      // hand it to the NIC
    return true;
}

int e1000_poll(void* buf, uint16_t cap)
{
    asm volatile("" : : : "memory");                              // re-read what the NIC wrote
    RxDesc& r = rx[rx_next];
    if ((r.status & 1) == 0) return 0;                            // DD: nothing new
    int len = r.length <= cap ? r.length : cap;
    memcpy(buf, rxbuf[rx_next], static_cast<size_t>(len));
    r.status = 0;
    wr(RDT, static_cast<uint32_t>(rx_next));                      // give the buffer back
    rx_next = (rx_next + 1) % NRX;
    return len;
}
