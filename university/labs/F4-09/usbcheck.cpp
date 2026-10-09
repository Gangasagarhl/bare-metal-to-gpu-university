// usbcheck.cpp - DR302 F4-09: compares usb.h (and the SCSI opcodes of bot.h) with
// Linux's <linux/usb/ch9.h> and <scsi/scsi.h> in the build container (tier 4).
#include <cstddef>
#include <cstdio>
#include <linux/usb/ch9.h>
#include <scsi/scsi.h>

// <scsi/scsi.h> defines the opcodes as macros with the same names as bot.h's constants:
// copy the values, then remove the macros before bot.h is included.
constexpr unsigned kTestUnitReady = TEST_UNIT_READY, kRequestSense = REQUEST_SENSE, kInquiry = INQUIRY,
                   kReadCapacity = READ_CAPACITY, kRead10 = READ_10, kWrite10 = WRITE_10;
#undef TEST_UNIT_READY
#undef REQUEST_SENSE
#undef INQUIRY
#undef READ_CAPACITY
#undef READ_10
#undef WRITE_10

#include "usb.h"
#include "bot.h"

namespace
{
int g_bad = 0;

void check(const char* name, unsigned ours, unsigned theirs)
{
    const bool ok = ours == theirs;
    std::printf("%-34s ours %4u  linux %4u  %s\n", name, ours, theirs, ok ? "ok" : "MISMATCH");
    if (!ok) {
        ++g_bad;
    }
}
}  // namespace

int main()
{
    check("USB_DIR_IN", usb::DIR_IN, USB_DIR_IN);
    check("USB_TYPE_CLASS", usb::TYPE_CLASS, USB_TYPE_CLASS);
    check("USB_RECIP_INTERFACE", usb::RECIP_INTERFACE, USB_RECIP_INTERFACE);
    check("USB_REQ_SET_ADDRESS", usb::REQ_SET_ADDRESS, USB_REQ_SET_ADDRESS);
    check("USB_REQ_GET_DESCRIPTOR", usb::REQ_GET_DESCRIPTOR, USB_REQ_GET_DESCRIPTOR);
    check("USB_REQ_SET_CONFIGURATION", usb::REQ_SET_CONFIGURATION, USB_REQ_SET_CONFIGURATION);
    check("USB_DT_DEVICE", usb::DT_DEVICE, USB_DT_DEVICE);
    check("USB_DT_CONFIG", usb::DT_CONFIG, USB_DT_CONFIG);
    check("USB_DT_STRING", usb::DT_STRING, USB_DT_STRING);
    check("USB_DT_INTERFACE", usb::DT_INTERFACE, USB_DT_INTERFACE);
    check("USB_DT_ENDPOINT", usb::DT_ENDPOINT, USB_DT_ENDPOINT);
    check("USB_ENDPOINT_XFER_BULK", usb::XFER_BULK, USB_ENDPOINT_XFER_BULK);
    check("USB_ENDPOINT_XFER_INT", usb::XFER_INT, USB_ENDPOINT_XFER_INT);
    check("USB_CLASS_HID", usb::CLASS_HID, USB_CLASS_HID);
    check("USB_CLASS_MASS_STORAGE", usb::CLASS_MASS_STORAGE, USB_CLASS_MASS_STORAGE);
    check("sizeof device descriptor", sizeof(usb::DeviceDescriptor), USB_DT_DEVICE_SIZE);
    check("sizeof config descriptor", sizeof(usb::ConfigDescriptor), USB_DT_CONFIG_SIZE);
    check("sizeof interface descriptor", sizeof(usb::InterfaceDescriptor), USB_DT_INTERFACE_SIZE);
    check("sizeof endpoint descriptor", sizeof(usb::EndpointDescriptor), USB_DT_ENDPOINT_SIZE);
    check("offsetof idVendor", offsetof(usb::DeviceDescriptor, idVendor),
          offsetof(struct usb_device_descriptor, idVendor));
    check("offsetof bMaxPacketSize0", offsetof(usb::DeviceDescriptor, bMaxPacketSize0),
          offsetof(struct usb_device_descriptor, bMaxPacketSize0));
    check("offsetof wTotalLength", offsetof(usb::ConfigDescriptor, wTotalLength),
          offsetof(struct usb_config_descriptor, wTotalLength));
    check("offsetof wMaxPacketSize", offsetof(usb::EndpointDescriptor, wMaxPacketSize),
          offsetof(struct usb_endpoint_descriptor, wMaxPacketSize));
    check("offsetof setup wLength", offsetof(usb::Setup, wLength), offsetof(struct usb_ctrlrequest, wLength));
    check("SCSI TEST UNIT READY", bot::TEST_UNIT_READY, kTestUnitReady);
    check("SCSI REQUEST SENSE", bot::REQUEST_SENSE, kRequestSense);
    check("SCSI INQUIRY", bot::INQUIRY, kInquiry);
    check("SCSI READ CAPACITY(10)", bot::READ_CAPACITY_10, kReadCapacity);
    check("SCSI READ(10)", bot::READ_10, kRead10);
    check("SCSI WRITE(10)", bot::WRITE_10, kWrite10);
    std::printf("%s: %d mismatches\n", g_bad == 0 ? "PASS" : "FAIL", g_bad);
    return g_bad == 0 ? 0 : 1;
}
