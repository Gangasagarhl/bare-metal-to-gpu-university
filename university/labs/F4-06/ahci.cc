// ahci.cc - DR301 F4-06: AHCI HBA driver. One command slot is used at a time; NCQ is the
// extension exercise. Structures live in static memory, which is physical memory here.
#include "ahci.h"
#include "ahci_regs.h"
#include "driver.h"
#include "kbase.h"
#include "../F4-03/irq.h"

namespace {
uintptr_t g_abar = 0;
alignas(1024) CmdHeader g_cmdlist[32];      // command list: 1 KiB aligned
alignas(256) uint8_t g_rfis[256];           // received-FIS area: 256-byte aligned
alignas(128) CmdTable g_table;              // the table of slot 0: 128-byte aligned
alignas(4096) uint8_t g_identify[512];
ahci::DiskInfo g_disks[1];
int g_ndisks = 0;
int g_port = -1;
volatile uint32_t g_irqs = 0;

uint32_t rd(uint32_t off) { return mmio_read<uint32_t>(g_abar + off); }
void wr(uint32_t off, uint32_t v) { mmio_write<uint32_t>(g_abar + off, v); }
uint32_t prd(int p, uint32_t off) { return rd(0x100 + 0x80 * p + off); }
void pwr(int p, uint32_t off, uint32_t v) { wr(0x100 + 0x80 * p + off, v); }

bool wait_clear(int p, uint32_t off, uint32_t bits, uint32_t ms)
{
    const uint64_t end = timer::ms() + ms;
    while (prd(p, off) & bits)
        if (timer::ms() > end) return false;
    return true;
}

// "Software Manipulation of Port DMA Engines": stop = clear ST, wait CR; clear FRE, wait FR.
bool port_stop(int p)
{
    pwr(p, port::CMD, prd(p, port::CMD) & ~port::CMD_ST);
    if (!wait_clear(p, port::CMD, port::CMD_CR, 500)) return false;
    pwr(p, port::CMD, prd(p, port::CMD) & ~port::CMD_FRE);
    return wait_clear(p, port::CMD, port::CMD_FR, 500);
}

// Start order of the spec: FRE first (the port may now receive FISes, among them the
// device's first D2H FIS with its signature), wait until the device is not busy, then ST.
bool port_start(int p)
{
    pwr(p, port::CMD, prd(p, port::CMD) | port::CMD_FRE);
    const bool ready = wait_clear(p, port::TFD, port::TFD_BSY | port::TFD_DRQ, 1000);
    pwr(p, port::CMD, prd(p, port::CMD) | port::CMD_ST);
    return ready;
}

// COMRESET: SCTL.DET = 1 for at least 1 ms, then 0; the link then comes back up.
// Returns the time until SSTS.DET showed "device present", or -1 after 'limit_ms'.
int port_reset(int p, uint32_t limit_ms)
{
    pwr(p, port::SCTL, (prd(p, port::SCTL) & ~0xFu) | 1);
    timer::sleep_ms(2);
    pwr(p, port::SCTL, prd(p, port::SCTL) & ~0xFu);
    const uint64_t t0 = timer::ms();
    while ((prd(p, port::SSTS) & port::SSTS_DET_MASK) != port::DET_PRESENT) {
        if (timer::ms() - t0 >= limit_ms) return -1;
        irq::wait();
    }
    pwr(p, port::SERR, 0xFFFFFFFF);         // the reset leaves diagnostic bits: clear them
    return static_cast<int>(timer::ms() - t0);
}

void ahci_isr()
{
    g_irqs = g_irqs + 1;
    const uint32_t is = rd(hba::IS);        // which ports want attention
    for (int p = 0; p < 32; ++p)
        if (is & (1u << p)) pwr(p, port::IS, prd(p, port::IS));   // RW1C: write back to clear
    wr(hba::IS, is);                        // then the HBA-level bits, also RW1C
}

// Build and issue one command in slot 0, wait for it. Returns 0 or -E_IO.
int issue(uint8_t command, uint64_t lba, uint32_t count, void* buf, uint32_t bytes, bool write)
{
    const int p = g_port;
    memset(&g_table, 0, sizeof g_table);
    uint8_t* f = g_table.cfis;
    f[0] = ata::FIS_REG_H2D;
    f[1] = 0x80;                            // C bit: this FIS carries a command
    f[2] = command;
    f[4] = static_cast<uint8_t>(lba); f[5] = static_cast<uint8_t>(lba >> 8); f[6] = static_cast<uint8_t>(lba >> 16);
    f[7] = 0x40;                            // device register: LBA mode
    f[8] = static_cast<uint8_t>(lba >> 24); f[9] = static_cast<uint8_t>(lba >> 32); f[10] = static_cast<uint8_t>(lba >> 40);
    f[12] = static_cast<uint8_t>(count); f[13] = static_cast<uint8_t>(count >> 8);
    uint16_t nprd = 0;
    if (bytes) {                            // one PRD entry per 4 KiB page of the buffer
        const auto base = reinterpret_cast<uintptr_t>(buf);
        for (uint32_t done = 0; done < bytes; done += 4096) {
            const uint32_t n = (bytes - done < 4096) ? bytes - done : 4096;
            g_table.prdt[nprd++] = Prd{static_cast<uint32_t>(base + done), 0, 0, n - 1};
        }
    }
    CmdHeader& h = g_cmdlist[0];
    h.flags = static_cast<uint16_t>(5 | (write ? 0x40 : 0));   // 5 dwords = 20-byte FIS
    h.prdtl = nprd;
    h.prdbc = 0;
    h.ctba = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_table));
    h.ctbau = 0;
    compiler_barrier();
    pwr(p, port::IS, 0xFFFFFFFF);
    pwr(p, port::CI, 1);                    // slot 0: go
    const uint64_t t0 = timer::ms();
    while (prd(p, port::CI) & 1) {
        if (prd(p, port::IS) & port::IS_TFES) break;            // task file error
        if (timer::ms() - t0 > 5000) break;
    }
    const uint32_t is = prd(p, port::IS), tfd = prd(p, port::TFD);
    if ((is & port::IS_TFES) || (tfd & port::TFD_ERR) || (prd(p, port::CI) & 1)) {
        kprintf("ahci: command 0x%02x failed: PxIS=0x%08x PxTFD=0x%08x (error register 0x%02x) PxSERR=0x%08x\n",
                command, is, tfd, (tfd >> 8) & 0xFF, prd(p, port::SERR));
        // recovery: stop the port, clear errors, start it again (a COMRESET would follow
        // if the device stayed busy)
        port_stop(p);
        pwr(p, port::SERR, 0xFFFFFFFF);
        pwr(p, port::IS, 0xFFFFFFFF);
        port_start(p);
        kprintf("ahci: port %d recovered: PxCMD=0x%08x PxTFD=0x%08x\n", p, prd(p, port::CMD), prd(p, port::TFD));
        return -E_IO;
    }
    return 0;
}

void ata_string(char* out, const uint8_t* id, int word, int words)
{
    // ATA strings store two characters per 16-bit word, the first in the high byte.
    int n = 0;
    for (int w = word; w < word + words; ++w) { out[n++] = static_cast<char>(id[2 * w + 1]); out[n++] = static_cast<char>(id[2 * w]); }
    while (n > 0 && out[n - 1] == ' ') --n;
    out[n] = 0;
}
}  // namespace

namespace ahci {
int init(PciAddr a, bool slow_probe)
{
    Bar bars[6];
    pci::size_bars(a, bars, 6);
    g_abar = static_cast<uintptr_t>(bars[5].addr);   // ABAR is BAR5
    pci::enable(a, pcireg::CMD_MEMORY | pcireg::CMD_MASTER);
    // 1. Reset the HBA, then switch it into AHCI mode.
    wr(hba::GHC, rd(hba::GHC) | hba::GHC_HR);
    while (rd(hba::GHC) & hba::GHC_HR) { }
    wr(hba::GHC, hba::GHC_AE);
    const uint32_t cap = rd(hba::CAP), pi = rd(hba::PI), vs = rd(hba::VS);
    kprintf("ahci: ABAR 0x%x version %x.%x, CAP=0x%08x (%u ports, %u command slots, 64-bit %s, NCQ %s), PI=0x%08x\n",
            static_cast<uint32_t>(g_abar), vs >> 16, vs & 0xFFFF, cap, (cap & hba::CAP_NP_MASK) + 1,
            ((cap >> hba::CAP_NCS_SHIFT) & 0x1F) + 1, (cap & hba::CAP_S64A) ? "yes" : "no",
            (cap & hba::CAP_SNCQ) ? "yes" : "no", pi);
    // 2. Each implemented port: is anything there?
    for (int p = 0; p < 32; ++p) {
        if (!(pi & (1u << p))) continue;
        port_stop(p);
        int link_ms;
        const uint64_t probe_t0 = timer::ms();
        if (slow_probe) {
            link_ms = port_reset(p, 1000);  // reset every port and wait the full second
        } else {
            // The firmware already brought the links up: read DET first and only reset
            // ports that report a device. An empty port costs one register read.
            link_ms = ((prd(p, port::SSTS) & port::SSTS_DET_MASK) == port::DET_PRESENT) ? port_reset(p, 1000) : -1;
        }
        if (link_ms < 0) { kprintf("ahci: port %d: SSTS=0x%03x no device (probe took %u ms)\n", p, prd(p, port::SSTS),
                                 static_cast<uint32_t>(timer::ms() - probe_t0)); continue; }
        if (g_ndisks == 1) { kprintf("ahci: port %d: device present, not used (one disk only)\n", p); continue; }
        // 3. Give the port its command list and FIS area, clear old status, start it.
        memset(g_cmdlist, 0, sizeof g_cmdlist);
        pwr(p, port::CLB, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(g_cmdlist)));
        pwr(p, port::CLBU, 0);
        pwr(p, port::FB, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(g_rfis)));
        pwr(p, port::FBU, 0);
        pwr(p, port::SERR, 0xFFFFFFFF);
        pwr(p, port::IS, 0xFFFFFFFF);
        const bool ready = port_start(p);
        const uint32_t sig = prd(p, port::SIG);     // valid once the first D2H FIS arrived
        kprintf("ahci: port %d: SSTS=0x%03x SIG=0x%08x %s (link up %d ms after COMRESET%s)\n", p, prd(p, port::SSTS),
                sig, sig == port::SIG_ATAPI ? "ATAPI (not handled)" : "SATA disk", link_ms, ready ? "" : ", still busy");
        if (sig == port::SIG_ATAPI || !ready) { port_stop(p); continue; }
        g_port = p;
        // 4. IDENTIFY DEVICE: 512 bytes describing the disk.
        if (issue(ata::IDENTIFY, 0, 0, g_identify, 512, false) != 0) continue;
        DiskInfo& d = g_disks[g_ndisks++];
        d.port = p;
        ata_string(d.serial, g_identify, 10, 10);
        ata_string(d.firmware, g_identify, 23, 4);
        ata_string(d.model, g_identify, 27, 20);
        uint64_t s = 0;
        for (int w = 3; w >= 0; --w) s = (s << 16) | (g_identify[2 * (100 + w)] | (g_identify[2 * (100 + w) + 1] << 8));
        d.sectors = s;
    }
    // 5. Interrupts: the PCI interrupt line the firmware assigned, through the PIC.
    const uint8_t line = pci::read8(a, pcireg::INTERRUPT_LINE);
    if (g_port >= 0 && line < 16) {
        irq::set_handler(line, ahci_isr);
        irq::unmask(line);
        if (line >= 8) irq::unmask(2);
        pwr(g_port, port::IE, port::IS_DHRS | port::IS_TFES);   // device-to-host FIS, errors
        wr(hba::GHC, rd(hba::GHC) | hba::GHC_IE);
        kprintf("ahci: interrupts on PCI line %u\n", line);
    }
    return g_ndisks;
}

const DiskInfo& disk(int i) { return g_disks[i]; }

int rw(bool write, uint64_t lba, uint32_t count, void* buf)
{
    return issue(write ? ata::WRITE_DMA_EXT : ata::READ_DMA_EXT, lba, count, buf, count * 512, write);
}

uint32_t interrupts() { return g_irqs; }

void dump_port(int p)
{
    kprintf("port %d: CMD=0x%08x IS=0x%08x TFD=0x%08x SSTS=0x%08x SERR=0x%08x CI=0x%08x\n", p,
            prd(p, port::CMD), prd(p, port::IS), prd(p, port::TFD), prd(p, port::SSTS), prd(p, port::SERR),
            prd(p, port::CI));
}
}  // namespace ahci
