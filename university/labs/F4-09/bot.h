// bot.h - DR302 F4-09: USB Mass Storage Bulk-Only Transport with SCSI block commands.
// CBW/CSW: USB Mass Storage Class Bulk-Only Transport specification; command layouts:
// SCSI Primary Commands (INQUIRY, TEST UNIT READY) and SCSI Block Commands (READ
// CAPACITY(10), READ(10), WRITE(10)). Opcodes checked against <scsi/scsi.h> by
// usbcheck.cpp; field layouts from memory, pending verification.
#pragma once
#include <stdint.h>

namespace bot {
constexpr uint8_t TEST_UNIT_READY = 0x00, REQUEST_SENSE = 0x03, INQUIRY = 0x12, READ_CAPACITY_10 = 0x25, READ_10 = 0x28,
                  WRITE_10 = 0x2A;
constexpr uint32_t CBW_SIGNATURE = 0x43425355u, CSW_SIGNATURE = 0x53425355u;   // "USBC", "USBS"
struct Cbw {
    uint32_t signature, tag, data_length;
    uint8_t flags, lun, cb_length, cb[16];
} __attribute__((packed));
struct Csw {
    uint32_t signature, tag, residue;
    uint8_t status;              // 0 passed, 1 failed, 2 phase error
} __attribute__((packed));
static_assert(sizeof(Cbw) == 31 && sizeof(Csw) == 13, "BOT wrapper sizes");
}

namespace usbh { struct Device; }

namespace bot {
bool start(usbh::Device& d);
// One SCSI command; returns the CSW status (0 = passed), or a negative transport error.
int command(const uint8_t* cdb, uint8_t cdb_len, void* data, uint32_t length, bool in);
bool read_capacity(uint32_t& last_lba, uint32_t& block_size);
int read(uint32_t lba, uint16_t blocks, void* buf);      // at most 8 blocks of 512 bytes
int write(uint32_t lba, uint16_t blocks, const void* buf);
// After a failed command: sense key, additional sense code and qualifier (fixed format).
bool request_sense(uint8_t& key, uint8_t& asc, uint8_t& ascq);
}
