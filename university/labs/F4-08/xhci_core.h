// xhci_core.h - DR302 F4-08: the core of an xHCI host-controller driver: reset, the
// command ring, one event ring (interrupter 0, signalled by MSI-X or polled), doorbells.
// F4-08 uses it to show MSI-X; F4-09 builds device enumeration and transfers on top.
// Register offsets, bit positions and TRB formats were written from the author's memory
// of the eXtensible Host Controller Interface for USB (xHCI) Specification ("Host
// Controller Capability Registers", "Host Controller Operational Registers", "Host
// Controller Runtime Registers", "TRB Types"); not opened in this build, confirmed only
// by QEMU's qemu-xhci model behaving as expected (see F4-08 and F4-09).
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"
#include "msi.h"

namespace xhci {
struct alignas(16) Trb {
    uint64_t param;
    uint32_t status;
    uint32_t control;            // bit 0 cycle, bits 15:10 TRB type
};
static_assert(sizeof(Trb) == 16, "a TRB is 16 bytes");

enum TrbType : uint32_t {
    NORMAL = 1, SETUP = 2, DATA = 3, STATUS = 4, LINK = 6, NOOP = 8,
    ENABLE_SLOT = 9, DISABLE_SLOT = 10, ADDRESS_DEVICE = 11, CONFIGURE_ENDPOINT = 12,
    EVALUATE_CONTEXT = 13, NOOP_COMMAND = 23,
    TRANSFER_EVENT = 32, COMMAND_COMPLETION = 33, PORT_STATUS_CHANGE = 34
};
constexpr uint32_t CC_SUCCESS = 1, CC_SHORT_PACKET = 13;
inline uint32_t trb_type(const Trb& t) { return (t.control >> 10) & 0x3F; }
inline uint32_t completion_code(const Trb& t) { return t.status >> 24; }
inline uint32_t event_slot(const Trb& t) { return t.control >> 24; }

// A producer ring (command ring or transfer ring): N TRBs, the last one a Link TRB back
// to the start that toggles the producer cycle state.
struct Ring {
    Trb* trb = nullptr;
    uint32_t n = 0, enq = 0;
    uint32_t cycle = 1;
    uint64_t iova = 0;           // the address the controller uses for trb[0]
};
void ring_init(Ring& r, Trb* mem, uint32_t n);
uint64_t ring_push(Ring& r, Trb t);          // returns the IOVA of the TRB written

struct Info {
    uintptr_t mmio = 0, op = 0, rt = 0, db = 0;
    uint8_t max_slots = 0, max_ports = 0;
    uint16_t max_intrs = 0, version = 0;
    uint32_t context_size = 32;              // 32 or 64 bytes (HCCPARAMS1.CSZ)
    uint32_t scratchpads = 0;
    PciAddr pci{};
    msix::Table msix{};
    uint8_t vector = 0;                      // 0: polled
};
const Info& info();

// Resets and starts the controller. vector != 0: interrupter 0 signals by MSI-X entry 0.
bool init(PciAddr a, uint8_t vector);
bool find(PciAddr& out);                     // class 0C/03/30
// Places one command on the command ring, rings doorbell 0, waits for its completion.
Trb command(Trb cmd, uint32_t timeout_ms = 1000);
// Waits for the next event that is not a command completion; false on timeout.
bool wait_event(Trb& ev, uint32_t timeout_ms);
void doorbell(uint8_t slot, uint32_t target);
uint32_t portsc(uint8_t port);               // ports are numbered from 1
void set_portsc(uint8_t port, uint32_t v);
uint32_t interrupts();                       // interrupter-0 interrupts handled so far
void* dcbaa();                               // device context base address array
uint64_t iova_of(const void* p, uint32_t bytes, bool device_writes);   // via dma::map
}
