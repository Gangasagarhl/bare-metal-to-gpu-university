// virtio.h - DR301 F4-05: modern virtio over PCI, split virtqueues.
// Structure layouts written from the author's memory of the OASIS "Virtual I/O Device
// (VIRTIO)" specification ("Basic Facilities of a Virtio Device", "Virtio Over PCI Bus");
// checked against Linux's <linux/virtio_pci.h>, <linux/virtio_ring.h> and
// <linux/virtio_config.h> by virtio_check.cc, and by QEMU's devices working.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../F4-02/pci.h"

namespace vio {
// device status bits
constexpr uint8_t S_ACKNOWLEDGE = 1, S_DRIVER = 2, S_DRIVER_OK = 4, S_FEATURES_OK = 8,
                  S_NEEDS_RESET = 0x40, S_FAILED = 0x80;
constexpr unsigned F_VERSION_1 = 32;           // feature bit: "modern" device
// PCI vendor capability types
constexpr uint8_t CAP_COMMON = 1, CAP_NOTIFY = 2, CAP_ISR = 3, CAP_DEVICE = 4;
// descriptor flags
constexpr uint16_t DESC_NEXT = 1, DESC_WRITE = 2;

struct CommonCfg {                             // in BAR memory; every access is volatile
    uint32_t device_feature_select, device_feature, driver_feature_select, driver_feature;
    uint16_t msix_config, num_queues;
    uint8_t device_status, config_generation;
    uint16_t queue_select, queue_size, queue_msix_vector, queue_enable, queue_notify_off;
    uint32_t queue_desc_lo, queue_desc_hi, queue_driver_lo, queue_driver_hi,
             queue_device_lo, queue_device_hi;
};
static_assert(sizeof(CommonCfg) == 56, "common configuration is 56 bytes");

struct Desc { uint64_t addr; uint32_t len; uint16_t flags, next; };
struct UsedElem { uint32_t id, len; };
static_assert(sizeof(Desc) == 16 && sizeof(UsedElem) == 8, "ring element sizes");

constexpr uint16_t QMAX = 128;                 // descriptors per queue in this driver

// One split virtqueue: descriptor table, driver ("available") ring, device ("used") ring.
struct Queue {
    alignas(4096) Desc desc[QMAX];
    alignas(2) struct { uint16_t flags, idx, ring[QMAX], used_event; } avail;
    alignas(4096) struct { uint16_t flags, idx; UsedElem ring[QMAX]; uint16_t avail_event; } used;
    uint16_t size = 0, free_head = 0, num_free = 0, last_used = 0;
    uint16_t next_free[QMAX];
    volatile uint16_t* notify = nullptr;
    uint16_t index = 0;
};

struct Device {
    PciAddr pci;
    volatile CommonCfg* common = nullptr;
    volatile uint8_t* isr = nullptr;
    volatile uint8_t* device_cfg = nullptr;
    uintptr_t notify_base = 0;
    uint32_t notify_mult = 0;
    uint64_t features = 0;                     // what both sides agreed on
};

// Transport: find the capabilities, reset, negotiate. 'wanted' = feature bits the driver
// understands; VERSION_1 is always required. Returns 0 or a negative error.
int init(Device& d, PciAddr a, uint64_t wanted);
int setup_queue(Device& d, Queue& q, uint16_t index);
void driver_ok(Device& d);
// Ring operations
int alloc_chain(Queue& q, int n);              // head index of n linked descriptors, or -1
void submit(Queue& q, uint16_t head);          // publish to the available ring
void kick(Queue& q);                           // notify the device
bool get_used(Queue& q, uint32_t& head, uint32_t& len);   // one completion, if any
void free_chain(Queue& q, uint16_t head);
}  // namespace vio
