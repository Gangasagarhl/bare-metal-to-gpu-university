// bot.cc - DR302 F4-09: Bulk-Only Transport: CBW out, data, CSW in.
#include "bot.h"
#include "usbh.h"
#include "../F4-01/kbase.h"

namespace {
usbh::Device* g_dev = nullptr;
usbh::Endpoint* g_in = nullptr;
usbh::Endpoint* g_out = nullptr;
uint32_t g_tag = 1;

[[maybe_unused]] void put_be32(uint8_t* p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }
uint32_t get_be32(const uint8_t* p) { return uint32_t{p[0]} << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

void put_lba(uint8_t* p, uint32_t lba)
{
#ifdef F409_LBA_LE
    memcpy(p, &lba, 4);                  // forensic build: the CPU's byte order, not SCSI's
#else
    put_be32(p, lba);                    // SCSI fields are big-endian
#endif
}
}  // namespace

namespace bot {
bool start(usbh::Device& d)
{
    const usb::InterfaceDescriptor* f = usbh::interface(d, 0);
    if (!f || f->bInterfaceClass != usb::CLASS_MASS_STORAGE || f->bInterfaceProtocol != 0x50) return false;
    if (usbh::configure(d, f->bInterfaceNumber) < 2) return false;
    g_dev = &d;
    g_in = usbh::find_ep(d, usb::XFER_BULK, true);
    g_out = usbh::find_ep(d, usb::XFER_BULK, false);
    return g_in && g_out;
}

int command(const uint8_t* cdb, uint8_t cdb_len, void* data, uint32_t length, bool in)
{
    Cbw w{};
    w.signature = CBW_SIGNATURE;
    w.tag = g_tag++;
    w.data_length = length;
    w.flags = in ? 0x80 : 0x00;
    w.cb_length = cdb_len;
    memcpy(w.cb, cdb, cdb_len);
    if (usbh::transfer(*g_dev, *g_out, &w, sizeof w, 1000) != sizeof w) return -1;
    if (length) {
        const int got = usbh::transfer(*g_dev, in ? *g_in : *g_out, data, length, 2000);
        if (got < 0) return -2;
    }
    Csw s{};
    if (usbh::transfer(*g_dev, *g_in, &s, sizeof s, 1000) != sizeof s) return -3;
    if (s.signature != CSW_SIGNATURE || s.tag != w.tag) return -4;
    return s.status;
}

bool read_capacity(uint32_t& last_lba, uint32_t& block_size)
{
    uint8_t cdb[10] = {READ_CAPACITY_10}, r[8];
    if (command(cdb, 10, r, 8, true) != 0) return false;
    last_lba = get_be32(r);
    block_size = get_be32(r + 4);
    return true;
}

int read(uint32_t lba, uint16_t blocks, void* buf)
{
    uint8_t cdb[10] = {READ_10};
    put_lba(cdb + 2, lba);
    cdb[7] = blocks >> 8;                // transfer length in blocks, big-endian
    cdb[8] = blocks & 0xFF;
    return command(cdb, 10, buf, blocks * 512u, true);
}

bool request_sense(uint8_t& key, uint8_t& asc, uint8_t& ascq)
{
    uint8_t cdb[6] = {REQUEST_SENSE, 0, 0, 0, 18, 0}, r[18] = {};
    if (command(cdb, 6, r, sizeof r, true) != 0) return false;
    key = r[2] & 0x0F;
    asc = r[12];
    ascq = r[13];
    return true;
}

int write(uint32_t lba, uint16_t blocks, const void* buf)
{
    uint8_t cdb[10] = {WRITE_10};
    put_lba(cdb + 2, lba);
    cdb[7] = blocks >> 8;
    cdb[8] = blocks & 0xFF;
    return command(cdb, 10, const_cast<void*>(buf), blocks * 512u, false);
}
}  // namespace bot
