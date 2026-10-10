// nvme_regs.h - DR301 F4-07: NVMe controller registers, queue entries and command codes.
// Written from the NVM Express Base Specification ("Controller Registers", "Submission
// Queue Entry", "Completion Queue Entry", "Admin Command Set") and the NVM Command Set
// Specification (Read, Write, Flush), not opened during this build: the offsets are
// confirmed only by QEMU's model behaving as expected (see the chapter's unverified box).
#pragma once
#include <stdint.h>

namespace nreg {
constexpr uint32_t CAP = 0x00, VS = 0x08, INTMS = 0x0C, INTMC = 0x10, CC = 0x14, CSTS = 0x1C;
constexpr uint32_t AQA = 0x24, ASQ = 0x28, ACQ = 0x30, DOORBELL_BASE = 0x1000;
// CAP (64 bits)
constexpr uint64_t cap_mqes(uint64_t c) { return (c & 0xFFFF) + 1; }      // max entries per queue
constexpr uint64_t cap_to_500ms(uint64_t c) { return (c >> 24) & 0xFF; }  // ready timeout, 500 ms units
constexpr uint64_t cap_dstrd(uint64_t c) { return (c >> 32) & 0xF; }      // doorbell stride: 4 << DSTRD bytes
constexpr uint64_t cap_css(uint64_t c) { return (c >> 37) & 0xFF; }       // command sets (bit 0 = NVM)
constexpr uint64_t cap_mpsmin(uint64_t c) { return (c >> 48) & 0xF; }     // min page = 4 KiB << MPSMIN
// CC
constexpr uint32_t CC_EN = 1u << 0;
constexpr uint32_t CC_IOSQES_64 = 6u << 16, CC_IOCQES_16 = 4u << 20;     // log2 of the entry sizes
// CSTS
constexpr uint32_t CSTS_RDY = 1u << 0, CSTS_CFS = 1u << 1;
}

// Submission queue entry: 64 bytes.
struct Sqe {
    uint32_t cdw0;               // opcode (7:0), command identifier CID (31:16)
    uint32_t nsid;
    uint32_t cdw2, cdw3;
    uint64_t mptr;
    uint64_t prp1, prp2;         // data pointer: two PRP entries
    uint32_t cdw10, cdw11, cdw12, cdw13, cdw14, cdw15;
};
static_assert(sizeof(Sqe) == 64, "an SQ entry is 64 bytes");

// Completion queue entry: 16 bytes.
struct Cqe {
    uint32_t dw0;                // command-specific result
    uint32_t dw1;
    uint16_t sq_head, sq_id;     // how far the controller has read that SQ
    uint16_t cid;
    uint16_t status;             // bit 0 = phase tag, bits 15:1 = status field
};
static_assert(sizeof(Cqe) == 16, "a CQ entry is 16 bytes");

namespace nop {
// admin commands
constexpr uint8_t DELETE_IO_SQ = 0x00, CREATE_IO_SQ = 0x01, DELETE_IO_CQ = 0x04, CREATE_IO_CQ = 0x05;
constexpr uint8_t IDENTIFY = 0x06, SET_FEATURES = 0x09;
constexpr uint32_t CNS_NAMESPACE = 0x00, CNS_CONTROLLER = 0x01, CNS_ACTIVE_NS_LIST = 0x02;
constexpr uint32_t FEAT_NUM_QUEUES = 0x07;
// NVM command set
constexpr uint8_t FLUSH = 0x00, WRITE = 0x01, READ = 0x02;
}
