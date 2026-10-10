// nvme.cc - DR301 F4-07: NVMe driver. Polled completions (MSI-X per queue arrives with
// C7), one admin queue pair and 1-8 I/O queue pairs, PRP lists for transfers over 8 KiB.
// All queue memory is static and identity-mapped (physical = virtual in the lab kernel).
#include "nvme.h"
#include "nvme_regs.h"
#include "driver.h"
#include "kbase.h"
#include "../F4-03/irq.h"

namespace {
constexpr int ADMIN_DEPTH = 32;
constexpr int MAX_IOQ = 8;
constexpr int PAGE = 4096;

struct alignas(PAGE) SqPage { Sqe e[64]; };             // 64 x 64 bytes = one page
struct alignas(PAGE) CqPage { Cqe e[64]; };             // 64 x 16 bytes, page aligned
alignas(PAGE) Sqe g_asq[ADMIN_DEPTH];
alignas(PAGE) Cqe g_acq[ADMIN_DEPTH];
SqPage g_iosq[MAX_IOQ];
CqPage g_iocq[MAX_IOQ];
alignas(PAGE) uint8_t g_id[PAGE];                         // Identify data
alignas(PAGE) uint64_t g_prp_list[PAGE / 8];              // one PRP list page (sync path)
alignas(PAGE) uint8_t g_slot_buf[nvme::SLOTS][PAGE];

struct Queue {
    Sqe* sq;
    Cqe* cq;
    uint16_t qid, size, sq_tail, cq_head;
    uint8_t phase;               // the phase value that marks a NEW completion entry
};

uintptr_t g_bar = 0;
uint32_t g_dstrd = 0;
Queue g_admin;
Queue g_io[MAX_IOQ];
int g_nq = 0;
uint16_t g_io_depth = 64;
bool g_no_phase = false;
nvme::CtrlInfo g_ctrl;
nvme::NsInfo g_ns[4];
int g_nns = 0;
uint16_t g_seq = 0;
uint16_t g_inflight[nvme::SLOTS];                         // CID per slot, 0 = free
uint32_t g_stale = 0;
uint32_t g_nsid_lba[8];                                   // LBA size per nsid (1..7)

uint32_t rd32(uint32_t off) { return mmio_read<uint32_t>(g_bar + off); }
void wr32(uint32_t off, uint32_t v) { mmio_write<uint32_t>(g_bar + off, v); }
uint64_t rd64(uint32_t off) { return rd32(off) | (uint64_t{rd32(off + 4)} << 32); }
void wr64(uint32_t off, uint64_t v) { wr32(off, static_cast<uint32_t>(v)); wr32(off + 4, static_cast<uint32_t>(v >> 32)); }
uint64_t phys(const void* p) { return reinterpret_cast<uintptr_t>(p); }

// Doorbells: SQ y tail at 0x1000 + (2y) * stride, CQ y head at 0x1000 + (2y + 1) * stride.
void ring_sq(const Queue& q) { wr32(nreg::DOORBELL_BASE + (2 * q.qid) * (4u << g_dstrd), q.sq_tail); }
void ring_cq(const Queue& q) { wr32(nreg::DOORBELL_BASE + (2 * q.qid + 1) * (4u << g_dstrd), q.cq_head); }

void submit(Queue& q, Sqe s)
{
    q.sq[q.sq_tail] = s;
    q.sq_tail = static_cast<uint16_t>((q.sq_tail + 1) % q.size);
    compiler_barrier();                                  // the entry is in memory before the doorbell
    ring_sq(q);
}

// Take the next completion if there is one. The controller flips the phase bit it
// writes each time it wraps around the queue, so an entry is new exactly when its
// phase bit equals the phase we expect for this pass.
bool poll(Queue& q, Cqe& out)
{
    Cqe& e = q.cq[q.cq_head];
    const uint16_t st = *reinterpret_cast<volatile uint16_t*>(&e.status);
    const uint8_t want = g_no_phase ? 1 : q.phase;        // the forensic bug: always expect 1
    if ((st & 1) != want) return false;
    compiler_barrier();
    out = e;
    out.status = st;
    if (++q.cq_head == q.size) {
        q.cq_head = 0;
        q.phase ^= 1;
    }
    ring_cq(q);                                          // tell the controller the slot is free
    return true;
}

uint16_t next_cid(uint16_t low5)
{
    g_seq = static_cast<uint16_t>(g_seq + 1);
    if ((g_seq & 0x3FF) == 0) g_seq = static_cast<uint16_t>(g_seq + 1);
    return static_cast<uint16_t>(((g_seq & 0x3FF) << 5) | low5);   // never 0
}

// Wait for the completion carrying 'cid' on queue q. Returns the status field (0 = success).
int wait_cid(Queue& q, uint16_t cid, uint32_t* dw0 = nullptr)
{
    const uint64_t t0 = timer::ms();
    for (;;) {
        Cqe c;
        if (poll(q, c)) {
            if (c.cid == cid) {
                if (dw0) *dw0 = c.dw0;
                return c.status >> 1;
            }
            g_stale = g_stale + 1;
            continue;
        }
        if (timer::ms() - t0 > 2000) {
            kprintf("nvme: timeout waiting for cid 0x%04x on queue %u (CSTS=0x%x)\n", cid, q.qid, rd32(nreg::CSTS));
            return -1;
        }
    }
}

int admin(uint8_t opcode, uint32_t nsid, uint64_t prp1, uint32_t cdw10, uint32_t cdw11, uint32_t* dw0 = nullptr)
{
    Sqe s{};
    const uint16_t cid = next_cid(31);
    s.cdw0 = opcode | (uint32_t{cid} << 16);
    s.nsid = nsid;
    s.prp1 = prp1;
    s.cdw10 = cdw10;
    s.cdw11 = cdw11;
    submit(g_admin, s);
    const int st = wait_cid(g_admin, cid, dw0);
    if (st != 0) kprintf("nvme: admin opcode 0x%02x failed, status 0x%x\n", opcode, st);
    return st;
}

void copy_str(char* out, const uint8_t* p, int n)
{
    for (int i = 0; i < n; ++i) out[i] = static_cast<char>(p[i]);
    while (n > 0 && out[n - 1] == ' ') --n;               // space padded, no terminator
    out[n] = 0;
}

int create_io_queues(int nq)
{
    for (int i = 0; i < nq; ++i) {
        Queue& q = g_io[i];
        q = Queue{g_iosq[i].e, g_iocq[i].e, static_cast<uint16_t>(i + 1), g_io_depth, 0, 0, 1};
        memset(&g_iocq[i], 0, sizeof g_iocq[i]);           // phase bits start at 0
        const uint32_t cdw10 = q.qid | (uint32_t{q.size - 1u} << 16);   // QSIZE is zero-based
        // the CQ first (physically contiguous, no interrupts: polled), then its SQ
        if (admin(nop::CREATE_IO_CQ, 0, phys(q.cq), cdw10, 0x1) != 0) return -E_IO;
        if (admin(nop::CREATE_IO_SQ, 0, phys(q.sq), cdw10, 0x1 | (uint32_t{q.qid} << 16)) != 0) return -E_IO;
    }
    g_nq = nq;
    return 0;
}

int delete_io_queues()
{
    for (int i = 0; i < g_nq; ++i)        // every SQ before the CQ it completes into
        if (admin(nop::DELETE_IO_SQ, 0, 0, g_io[i].qid, 0) != 0) return -E_IO;
    for (int i = 0; i < g_nq; ++i)
        if (admin(nop::DELETE_IO_CQ, 0, 0, g_io[i].qid, 0) != 0) return -E_IO;
    g_nq = 0;
    return 0;
}

// Fill PRP1/PRP2 for a page-aligned buffer of 'bytes'.
void set_prps(Sqe& s, const void* buf, uint32_t bytes, uint64_t* list)
{
    const uint64_t base = phys(buf);
    const uint32_t pages = (bytes + PAGE - 1) / PAGE;
    s.prp1 = base;
    if (pages == 1) {
        s.prp2 = 0;
    } else if (pages == 2) {
        s.prp2 = base + PAGE;                              // the second page itself
    } else {
        for (uint32_t i = 1; i < pages; ++i) list[i - 1] = base + uint64_t{i} * PAGE;
        s.prp2 = phys(list);                               // a pointer to the list of the rest
    }
}

Sqe io_cmd(bool write, uint32_t nsid, uint64_t lba, uint32_t nblocks, uint16_t cid)
{
    Sqe s{};
    s.cdw0 = (write ? nop::WRITE : nop::READ) | (uint32_t{cid} << 16);
    s.nsid = nsid;
    s.cdw10 = static_cast<uint32_t>(lba);                  // starting LBA, 64 bits
    s.cdw11 = static_cast<uint32_t>(lba >> 32);
    s.cdw12 = nblocks - 1;                                 // number of blocks, zero-based
    return s;
}
}  // namespace

namespace nvme {
int init(PciAddr a, int nqueues, bool no_phase)
{
    g_no_phase = no_phase;
    g_io_depth = no_phase ? 16 : 64;                       // a small queue wraps sooner
    Bar bars[6];
    pci::size_bars(a, bars, 6);
    if (!bars[0].addr || bars[0].io || bars[0].addr >> 32) return -E_NODEV;   // need MMIO below 4 GiB
    g_bar = static_cast<uintptr_t>(bars[0].addr);
    // memory decoding and bus mastering on; INTx off: this driver polls
    pci::enable(a, pcireg::CMD_MEMORY | pcireg::CMD_MASTER | pcireg::CMD_INTX_DISABLE);
    const uint64_t cap = rd64(nreg::CAP);
    const uint32_t vs = rd32(nreg::VS);
    g_dstrd = static_cast<uint32_t>(nreg::cap_dstrd(cap));
    const uint32_t timeout_ms = static_cast<uint32_t>(nreg::cap_to_500ms(cap)) * 500;
    kprintf("nvme: BAR0 0x%x, CAP=0x%08x%08x: MQES+1=%u, DSTRD=%u (doorbell stride %u bytes), TO=%u ms, "
            "CSS=0x%x, MPSMIN=%u; version %u.%u.%u\n",
            static_cast<uint32_t>(g_bar), static_cast<uint32_t>(cap >> 32), static_cast<uint32_t>(cap),
            static_cast<uint32_t>(nreg::cap_mqes(cap)), g_dstrd, 4u << g_dstrd, timeout_ms,
            static_cast<uint32_t>(nreg::cap_css(cap)), static_cast<uint32_t>(nreg::cap_mpsmin(cap)), vs >> 16,
            (vs >> 8) & 0xFF, vs & 0xFF);
    if (nreg::cap_mpsmin(cap) != 0 || !(nreg::cap_css(cap) & 1)) return -E_NODEV;   // need 4 KiB pages, NVM set
    // 1. Disable, and wait until the controller says it is no longer ready.
    if (rd32(nreg::CC) & nreg::CC_EN) {
        wr32(nreg::CC, rd32(nreg::CC) & ~nreg::CC_EN);
        const uint64_t t0 = timer::ms();
        while (rd32(nreg::CSTS) & nreg::CSTS_RDY)
            if (timer::ms() - t0 > timeout_ms) return -E_TIMEDOUT;
    }
    // 2. The admin queue pair: sizes (zero-based) and physical addresses.
    memset(g_asq, 0, sizeof g_asq);
    memset(g_acq, 0, sizeof g_acq);
    g_admin = Queue{g_asq, g_acq, 0, ADMIN_DEPTH, 0, 0, 1};
    wr32(nreg::AQA, (uint32_t{ADMIN_DEPTH - 1} << 16) | (ADMIN_DEPTH - 1));
    wr64(nreg::ASQ, phys(g_asq));
    wr64(nreg::ACQ, phys(g_acq));
    // 3. Enable: NVM command set, 4 KiB pages, 64-byte SQ entries, 16-byte CQ entries.
    wr32(nreg::CC, nreg::CC_IOSQES_64 | nreg::CC_IOCQES_16 | nreg::CC_EN);
    const uint64_t t0 = timer::ms();
    while (!(rd32(nreg::CSTS) & nreg::CSTS_RDY)) {
        if (rd32(nreg::CSTS) & nreg::CSTS_CFS) return -E_IO;
        if (timer::ms() - t0 > timeout_ms) return -E_TIMEDOUT;
    }
    kprintf("nvme: controller ready %u ms after CC.EN=1\n", static_cast<uint32_t>(timer::ms() - t0));
    // 4. Identify Controller.
    if (admin(nop::IDENTIFY, 0, phys(g_id), nop::CNS_CONTROLLER, 0) != 0) return -E_IO;
    g_ctrl.vid = static_cast<uint16_t>(g_id[0] | (g_id[1] << 8));
    copy_str(g_ctrl.serial, g_id + 4, 20);
    copy_str(g_ctrl.model, g_id + 24, 40);
    copy_str(g_ctrl.firmware, g_id + 64, 8);
    g_ctrl.mdts = g_id[77];
    memcpy(&g_ctrl.nn, g_id + 516, 4);
    // 5. The active namespace list, then Identify Namespace for each.
    if (admin(nop::IDENTIFY, 0, phys(g_id), nop::CNS_ACTIVE_NS_LIST, 0) != 0) return -E_IO;
    uint32_t ids[4] = {};
    memcpy(ids, g_id, sizeof ids);
    g_nns = 0;
    for (int i = 0; i < 4 && ids[i]; ++i) {
        if (admin(nop::IDENTIFY, ids[i], phys(g_id), nop::CNS_NAMESPACE, 0) != 0) return -E_IO;
        NsInfo& n = g_ns[g_nns++];
        n.nsid = ids[i];
        memcpy(&n.nsze, g_id, 8);
        const uint8_t format = g_id[26] & 0xF;             // FLBAS: which LBA format is in use
        const uint8_t lbads = g_id[128 + 4 * format + 2];  // LBAF[format] bits 23:16
        n.lba_bytes = 1u << lbads;
        if (n.nsid < 8) g_nsid_lba[n.nsid] = n.lba_bytes;
    }
    // 6. Ask for I/O queues, then create them.
    uint32_t got = 0;
    if (admin(nop::SET_FEATURES, 0, 0, nop::FEAT_NUM_QUEUES, (MAX_IOQ - 1) | ((MAX_IOQ - 1) << 16), &got) != 0)
        return -E_IO;
    kprintf("nvme: asked for %d I/O queue pairs, controller allows %u SQs and %u CQs\n", MAX_IOQ,
            (got & 0xFFFF) + 1, (got >> 16) + 1);
    return create_io_queues(nqueues);
}

const CtrlInfo& ctrl() { return g_ctrl; }
int namespaces() { return g_nns; }
const NsInfo& ns(int i) { return g_ns[i]; }
int queues() { return g_nq; }
uint32_t stale_completions() { return g_stale; }

int set_queues(int nqueues)
{
    const int rc = delete_io_queues();
    return rc ? rc : create_io_queues(nqueues);
}

int rw(uint32_t nsid, bool write, uint64_t lba, void* buf, uint32_t bytes)
{
    const uint32_t lb = (nsid < 8) ? g_nsid_lba[nsid] : 0;
    if (!lb || bytes == 0 || bytes % lb || bytes > 64 * 1024 || (phys(buf) & (PAGE - 1))) return -E_INVAL;
    const uint16_t cid = next_cid(30);                      // sync and async I/O never overlap in time
    Sqe s = io_cmd(write, nsid, lba, bytes / lb, cid);
    set_prps(s, buf, bytes, g_prp_list);
    submit(g_io[0], s);
    const int st = wait_cid(g_io[0], cid);
    if (st != 0) {
        kprintf("nvme: %s of %u bytes at LBA %lu failed, status 0x%x\n", write ? "write" : "read", bytes, lba, st);
        return -E_IO;
    }
    return 0;
}

int flush(uint32_t nsid)
{
    const uint16_t cid = next_cid(30);
    Sqe s{};
    s.cdw0 = nop::FLUSH | (uint32_t{cid} << 16);
    s.nsid = nsid;
    submit(g_io[0], s);
    return wait_cid(g_io[0], cid) == 0 ? 0 : -E_IO;
}

uint8_t* slot_buffer(int slot) { return g_slot_buf[slot]; }

void start(int slot, bool write, uint32_t nsid, uint64_t lba)
{
    const uint16_t cid = next_cid(static_cast<uint16_t>(slot));
    g_inflight[slot] = cid;
    Sqe s = io_cmd(write, nsid, lba, PAGE / g_nsid_lba[nsid], cid);
    s.prp1 = phys(g_slot_buf[slot]);
    submit(g_io[slot % g_nq], s);
}

int wait_any(uint16_t& status)
{
    const uint64_t t0 = timer::ms();
    for (int i = 0;; i = (i + 1) % g_nq) {
        Cqe c;
        if (poll(g_io[i], c)) {
            const int slot = c.cid & 31;
            if (slot >= SLOTS || g_inflight[slot] != c.cid) {
                // a completion for a command that is not in flight: an old entry read again
                if (g_stale < 5)
                    kprintf("nvme: STALE completion on queue %u: cid 0x%04x, slot %d holds 0x%04x, SQ head %u\n",
                            g_io[i].qid, c.cid, slot, g_inflight[slot], c.sq_head);
                g_stale = g_stale + 1;
                continue;
            }
            g_inflight[slot] = 0;
            status = static_cast<uint16_t>(c.status >> 1);
            return slot;
        }
        if (timer::ms() - t0 > 2000) {
            kprintf("nvme: no completion for 2000 ms; CSTS=0x%x\n", rd32(nreg::CSTS));
            return -1;
        }
    }
}
}  // namespace nvme
