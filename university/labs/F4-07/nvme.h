// nvme.h - DR301 F4-07: an NVMe driver, polled, with up to 8 I/O queue pairs.
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"

namespace nvme {
struct CtrlInfo {
    char serial[21], model[41], firmware[9];
    uint32_t nn;                 // number of namespaces
    uint8_t mdts;                // max transfer = 2^mdts pages (0 = no limit)
    uint16_t vid;
};
struct NsInfo {
    uint32_t nsid;
    uint64_t nsze;               // size in logical blocks
    uint32_t lba_bytes;          // 1 << LBADS
};

// nqueues = I/O queue pairs to create (1..8). no_phase = the forensic lab's bug.
int init(PciAddr a, int nqueues, bool no_phase);
const CtrlInfo& ctrl();
int namespaces();                                 // active namespaces found
const NsInfo& ns(int i);
// Synchronous transfer on I/O queue 1: 'bytes' (a multiple of the LBA size, at most
// 64 KiB) at 'lba' of namespace 'nsid'. Uses a PRP list when it spans more than 2 pages.
int rw(uint32_t nsid, bool write, uint64_t lba, void* buf, uint32_t bytes);
int flush(uint32_t nsid);
// Asynchronous 4 KiB requests for the random-I/O test: slot s goes to queue (s % nq) + 1.
constexpr int SLOTS = 32;
uint8_t* slot_buffer(int slot);
void start(int slot, bool write, uint32_t nsid, uint64_t lba);
int wait_any(uint16_t& status);                  // the finished slot, or -1 after a timeout
uint32_t stale_completions();                    // completions whose CID was not in flight
int queues();
// Recreate the I/O queues with a different count (deletes the old ones first).
int set_queues(int nqueues);
}
