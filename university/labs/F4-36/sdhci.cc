// sdhci.cc - F4-36: an SD Host Controller (SDHCI) driver, programmed I/O, polling, one block
// at a time; the SD card initialisation sequence; block read and write; a self-test that runs
// only if the devicetree's /chosen node asks for it (it writes to the card).
//
// Register offsets and bits after the SD Association's "SD Host Controller Simplified
// Specification"; commands and responses after the "Physical Layer Simplified Specification"
// (titles only - not opened during this build; check every number before reuse). All accesses
// are 32 bits wide: the BCM2835's controller is described in Linux's driver as accepting only
// 32-bit accesses (not verified here), and 32-bit accesses also work on QEMU's model.
#include "../F4-35/dm.h"
#include "../F4-32/bootimg.h"

namespace {

// Register offsets (32-bit words).
constexpr uint32_t kBlkSizeCount = 0x04;   // block size (bits 11:0), block count (31:16)
constexpr uint32_t kArgument = 0x08;
constexpr uint32_t kXferCmd = 0x0c;        // transfer mode (15:0), command (31:16)
constexpr uint32_t kResp0 = 0x10;          // response bits 31:0; then 0x14, 0x18, 0x1c
constexpr uint32_t kData = 0x20;           // buffer data port
constexpr uint32_t kPresent = 0x24;        // present state
constexpr uint32_t kHostPower = 0x28;      // host control 1 (7:0), power control (15:8)
constexpr uint32_t kClockReset = 0x2c;     // clock control (15:0), timeout (23:16), reset (31:24)
constexpr uint32_t kIntStatus = 0x30;      // normal (15:0) and error (31:16) status
constexpr uint32_t kIntEnable = 0x34;      // which status bits are latched
constexpr uint32_t kCaps = 0x40;
constexpr uint32_t kVersion = 0xfc;        // host controller version in bits 31:16

// Interrupt status bits.
constexpr uint32_t kCmdDone = 1u << 0;
constexpr uint32_t kXferDone = 1u << 1;
constexpr uint32_t kWriteReady = 1u << 4;
constexpr uint32_t kReadReady = 1u << 5;
constexpr uint32_t kError = 1u << 15;

// Response types in the command register: none, 136-bit, 48-bit, 48-bit with busy, plus the
// CRC check (bit 3), index check (bit 4) and data present (bit 5) flags.
constexpr uint32_t kRspNone = 0;
constexpr uint32_t kRsp136 = 1 | (1u << 3);
constexpr uint32_t kRsp48 = 2 | (1u << 3) | (1u << 4);
constexpr uint32_t kRsp48NoCrc = 2;        // R3 (OCR) carries no valid CRC
constexpr uint32_t kRsp48Busy = 3 | (1u << 3) | (1u << 4);
constexpr uint32_t kDataPresent = 1u << 5;

struct Card {
    uint64_t base = 0;
    uint32_t rca = 0;
    bool block_addressing = false;   // SDHC/SDXC: argument = block number; SDSC: byte address
    uint64_t blocks = 0;
};
Card g_card;

uint32_t rd(uint32_t off) { return k::rd32(g_card.base + off); }
void wr(uint32_t off, uint32_t v) { k::wr32(g_card.base + off, v); }

// Waits until one of `bits` (or the error bit) is set in the status register; 0 on time-out.
uint32_t wait_status(uint32_t bits, uint64_t us)
{
    uint64_t end = k::counter() + us * k::counter_freq() / 1000000;
    while (k::counter() < end) {
        uint32_t s = rd(kIntStatus);
        if ((s & (bits | kError)) != 0) {
            return s;
        }
    }
    return 0;
}

// Sends one command and waits for "command complete". Returns false on error or time-out.
bool command(uint32_t index, uint32_t arg, uint32_t flags, uint32_t mode = 0)
{
    uint64_t end = k::counter() + k::counter_freq() / 10;
    while ((rd(kPresent) & 1u) != 0 && k::counter() < end) {   // command line busy
    }
    wr(kIntStatus, 0xffffffff);                 // write 1 to clear every status bit
    wr(kArgument, arg);
    wr(kXferCmd, mode | ((index << 8 | flags) << 16));
    uint32_t s = wait_status(kCmdDone, 100000);
    if (s == 0 || (s & kError) != 0) {
        k::printf("    CMD%u failed: status 0x%08x, present state 0x%08x\n", index, s, rd(kPresent));
        wr(kIntStatus, 0xffffffff);
        return false;
    }
    wr(kIntStatus, kCmdDone);
    return true;
}

bool app_command(uint32_t index, uint32_t arg, uint32_t flags)
{
    return command(55, g_card.rca << 16, kRsp48) && command(index, arg, flags);
}

bool reset_and_clock()
{
    wr(kClockReset, 1u << 24);                  // software reset for all
    uint64_t end = k::counter() + k::counter_freq() / 10;
    while ((rd(kClockReset) & (1u << 24)) != 0) {
        if (k::counter() > end) {
            return false;
        }
    }
    wr(kHostPower, (0x7u << 9 | 1u << 8));      // power control: 3.3 V, bus power on
    // clock control: divider 0x80 in bits 15:8, internal clock enable (bit 0)
    wr(kClockReset, (0xeu << 16) | (0x80u << 8) | 1u);
    end = k::counter() + k::counter_freq() / 10;
    while ((rd(kClockReset) & 2u) == 0) {       // internal clock stable
        if (k::counter() > end) {
            return false;
        }
    }
    wr(kClockReset, rd(kClockReset) | 4u);      // SD clock enable
    wr(kIntEnable, 0xffffffff);
    return true;
}

bool card_init()
{
    if (!command(0, 0, kRspNone)) {
        return false;
    }
    bool v2 = command(8, 0x1aa, kRsp48) && (rd(kResp0) & 0xfff) == 0x1aa;
    k::printf("    CMD8 (interface condition): %s\n", v2 ? "echo 0x1aa, version 2.00 or later card" : "no answer");
    uint32_t ocr = 0;
    for (int tries = 0; tries < 100; ++tries) {
        // ACMD41: HCS (bit 30) only if CMD8 answered; voltage window 3.2-3.4 V (bits 20, 21)
        if (!app_command(41, (v2 ? 1u << 30 : 0) | 0x00300000u, kRsp48NoCrc)) {
            return false;
        }
        ocr = rd(kResp0);
        if ((ocr & (1u << 31)) != 0) {
            break;                              // power-up finished
        }
        k::delay_us(10000);
    }
    g_card.block_addressing = (ocr & (1u << 30)) != 0;
    k::printf("    ACMD41: OCR 0x%08x -> %s\n", ocr,
              g_card.block_addressing ? "SDHC/SDXC (block addressing)" : "SDSC (byte addressing)");
    if (!command(2, 0, kRsp136)) {
        return false;
    }
    // The controller stores the 136-bit response without its CRC byte: bits 119:0 of the
    // register file hold bits 127:8 of the CID. The product name (CID bits 103:64) therefore
    // sits in register bits 95:56.
    uint32_t r1 = rd(kResp0 + 4);
    uint32_t r2 = rd(kResp0 + 8);
    char pnm[6] = {static_cast<char>(r2 >> 24), static_cast<char>(r2 >> 16), static_cast<char>(r2 >> 8),
                   static_cast<char>(r2), static_cast<char>(r1 >> 24), '\0'};
    k::printf("    CMD2 (CID): manufacturer 0x%02x, product name \"%s\"\n", (rd(kResp0 + 12) >> 16) & 0xff, pnm);
    if (!command(3, 0, kRsp48)) {
        return false;
    }
    g_card.rca = rd(kResp0) >> 16;
    if (!command(9, g_card.rca << 16, kRsp136)) {
        return false;
    }
    uint32_t c0 = rd(kResp0);
    uint32_t c1 = rd(kResp0 + 4);
    uint32_t c2 = rd(kResp0 + 8);
    uint32_t c3 = rd(kResp0 + 12);
    uint32_t structure = c3 >> 22 & 3;          // CSD bits 127:126
    if (structure == 1) {                        // CSD version 2.0: C_SIZE = CSD bits 69:48
        uint32_t c_size = (c1 >> 8) & 0x3fffff;
        g_card.blocks = (uint64_t{c_size} + 1) * 1024;
    } else {                                     // version 1.0: C_SIZE 73:62, C_SIZE_MULT 49:47,
        uint32_t read_bl_len = (c2 >> 8) & 0xf;  // READ_BL_LEN 83:80
        uint32_t c_size = ((c2 & 0x3) << 10) | (c1 >> 22);
        uint32_t mult = (c1 >> 7) & 0x7;
        uint64_t bytes = (uint64_t{c_size} + 1) << (mult + 2 + read_bl_len);
        g_card.blocks = bytes / 512;
    }
    (void)c0;
    k::printf("    CMD3: RCA 0x%04x; CMD9 (CSD): structure version %u.0, %lu blocks of 512 bytes (%lu MiB)\n",
              g_card.rca, structure + 1, g_card.blocks, g_card.blocks >> 11);
    if (!command(7, g_card.rca << 16, kRsp48Busy)) {
        return false;
    }
    return command(16, 512, kRsp48);             // block length 512 (fixed anyway on SDHC)
}

// Reads or writes one 512-byte block with the CPU moving every word through the data port.
bool transfer(uint64_t lba, uint32_t* buf, bool write)
{
    wr(kBlkSizeCount, (1u << 16) | 512u);
    uint32_t arg = static_cast<uint32_t>(g_card.block_addressing ? lba : lba * 512);
    uint32_t mode = write ? 0u : (1u << 4);      // transfer mode bit 4: direction card-to-host
    if (!command(write ? 24 : 17, arg, kRsp48 | kDataPresent, mode)) {
        return false;
    }
    uint32_t s = wait_status(write ? kWriteReady : kReadReady, 200000);
    if (s == 0 || (s & kError) != 0) {
        k::printf("    data phase failed: status 0x%08x\n", s);
        return false;
    }
    wr(kIntStatus, write ? kWriteReady : kReadReady);
    for (int i = 0; i < 128; ++i) {
        if (write) {
            wr(kData, buf[i]);
        } else {
            buf[i] = rd(kData);
        }
    }
    s = wait_status(kXferDone, 200000);
    wr(kIntStatus, 0xffffffff);
    return s != 0 && (s & kError) == 0;
}

uint32_t g_buf[128];

// The pattern the host-side checker (verify_image.py) also computes: block `lba`, word i.
uint32_t pattern(uint64_t lba, int i) { return static_cast<uint32_t>(lba * 0x9e3779b1u) ^ (i * 0x01010101u); }

void self_test(uint32_t first, uint32_t count)
{
    // 1. read block 0 (the partition table written by the host) and check its signature
    if (!transfer(0, g_buf, false)) {
        return;
    }
    const auto* b = reinterpret_cast<const uint8_t*>(g_buf);
    k::printf("    block 0: signature 0x%02x%02x, CRC-32 0x%08x\n", b[510], b[511], crc32(b, 512));
    // 2. write `count` blocks at pseudo-random places inside the scratch area, then read back
    uint32_t seed = 12345;
    int good = 0;
    for (uint32_t n = 0; n < count; ++n) {
        seed = seed * 1103515245u + 12345u;
        uint64_t lba = first + (seed >> 8) % (count * 4);
        for (int i = 0; i < 128; ++i) {
            g_buf[i] = pattern(lba, i);
        }
        uint32_t want = crc32(reinterpret_cast<const uint8_t*>(g_buf), 512);
        if (!transfer(lba, g_buf, true)) {
            break;
        }
        for (int i = 0; i < 128; ++i) {
            g_buf[i] = 0;
        }
        if (!transfer(lba, g_buf, false)) {
            break;
        }
        good += crc32(reinterpret_cast<const uint8_t*>(g_buf), 512) == want ? 1 : 0;
    }
    k::printf("    self-test: %d of %u blocks written, read back and matched (scratch area from block %u)\n",
              good, count, first);
}

dm::Probe sdhci_probe(dm::Device& dev)
{
    fdt::Prop p;
    if (dm::tree().prop(dev.node, "dr403,sd-pins", p)) {     // route the card's pins here first
        dm::Probe r = dm::pins_set(dev, "dr403,sd-pins");
        if (r != dm::Probe::kOk) {
            return r;
        }
    }
    g_card.base = dev.base;
    k::printf("  sdhci %s: version register 0x%04x, capabilities 0x%08x\n", dm::path(dev), rd(kVersion) >> 16,
              rd(kCaps));
    if (!reset_and_clock()) {
        dev.why = "controller did not leave reset";
        return dm::Probe::kFail;
    }
    uint32_t present = rd(kPresent);
    k::printf("    present state 0x%08x: card %s\n", present, (present & (1u << 16)) != 0 ? "inserted" : "NOT inserted");
    if ((present & (1u << 16)) == 0) {
        dev.why = "no card";
        return dm::Probe::kFail;
    }
    if (!card_init()) {
        dev.why = "card initialisation failed";
        return dm::Probe::kFail;
    }
    // Writing to a storage device is opt-in: only a /chosen property names a scratch area.
    int chosen = dm::tree().find_path("/chosen");
    if (chosen != fdt::kNone && dm::tree().prop(chosen, "dr403,sd-selftest", p) && p.cells() == 2) {
        self_test(p.u32(0), p.u32(1));
    } else {
        k::printf("    no self-test requested in /chosen: the card is only read\n");
    }
    return dm::Probe::kOk;
}
DR403_DRIVER(sdhci, "sdhci-bcm2835", sdhci_probe, "brcm,bcm2835-sdhci");

}  // namespace
