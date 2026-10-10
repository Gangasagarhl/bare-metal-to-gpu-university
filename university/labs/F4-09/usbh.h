// usbh.h - DR302 F4-09: USB device management on top of the F4-08 xHCI core:
// port reset, slots, address assignment, control/bulk/interrupt transfers, configuration.
// Context and TRB layouts from the xHCI Specification (sections "Slot Context",
// "Endpoint Context", "Input Control Context", "Transfer TRBs"), written from memory and
// confirmed only by QEMU's qemu-xhci model; pending verification.
#pragma once
#include <stdint.h>
#include "../F4-08/xhci_core.h"
#include "usb.h"

namespace usbh {
constexpr int MAX_DEVICES = 4, MAX_EPS = 4;
struct Endpoint {
    uint8_t address = 0;         // bEndpointAddress (bit 7 = IN)
    uint8_t type = 0;            // usb::XFER_*
    uint8_t dci = 0;             // device context index = 2 * number + IN
    uint16_t mps = 0;
    uint8_t interval = 0;        // bInterval as described
    xhci::Ring ring{};
};
struct Device {
    bool used = false;
    uint8_t slot = 0, port = 0, speed = 0;
    usb::DeviceDescriptor dd{};
    uint8_t config[256]{};       // the whole configuration descriptor set
    uint16_t config_len = 0;
    xhci::Ring ep0{};
    Endpoint eps[MAX_EPS];
    int neps = 0;
    uint8_t* in_ctx = nullptr;   // input context (driver writes)
    uint8_t* out_ctx = nullptr;  // device context (controller writes)
    uint8_t* xfer = nullptr;     // this device's DMA bounce page (4 KiB)
};

const char* speed_name(uint8_t s);
void print_protocols();                         // xHCI Supported Protocol capabilities
// Resets the port, enables a slot, addresses the device and reads its descriptors.
Device* attach(uint8_t port);
void detach(Device& d);                         // Disable Slot, forget the device
// Standard or class request on endpoint 0. 'data' (at most 4 KiB) is copied through the
// device's DMA page. Returns bytes transferred, or -completion code on failure.
int control(Device& d, uint8_t reqtype, uint8_t req, uint16_t value, uint16_t index, void* data,
            uint16_t length);
// Configures the endpoints listed in the configuration descriptor for interface 'iface'
// and sends SET_CONFIGURATION. Returns the number of endpoints configured.
int configure(Device& d, uint8_t iface);
Endpoint* find_ep(Device& d, uint8_t type, bool in);
// One bulk or interrupt transfer of at most 4 KiB; returns bytes or -completion code;
// -1000 on timeout.
int transfer(Device& d, Endpoint& ep, void* data, uint32_t length, uint32_t timeout_ms);
void print_descriptors(const Device& d);
const usb::InterfaceDescriptor* interface(const Device& d, int n);   // n-th interface, or null
// Waits for a port status change event; returns the port number or 0 on timeout.
uint8_t wait_port_change(uint32_t timeout_ms);
}
