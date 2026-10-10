// usb.h - DR302 F4-09: USB framework definitions shared by the host-controller driver and
// the class drivers. Standard requests and descriptors: USB 2.0 Specification, chapter 9
// ("USB Device Framework"); class codes: USB-IF class definitions. Values checked against
// Linux's <linux/usb/ch9.h> by usbcheck.cpp; the rest pending verification.
#pragma once
#include <stdint.h>

namespace usb {
// bmRequestType
constexpr uint8_t DIR_IN = 0x80, DIR_OUT = 0x00;
constexpr uint8_t TYPE_STANDARD = 0x00, TYPE_CLASS = 0x20;
constexpr uint8_t RECIP_DEVICE = 0x00, RECIP_INTERFACE = 0x01, RECIP_ENDPOINT = 0x02;
// bRequest (standard)
constexpr uint8_t REQ_GET_STATUS = 0x00, REQ_SET_ADDRESS = 0x05, REQ_GET_DESCRIPTOR = 0x06,
                  REQ_SET_CONFIGURATION = 0x09;
// descriptor types
constexpr uint8_t DT_DEVICE = 0x01, DT_CONFIG = 0x02, DT_STRING = 0x03, DT_INTERFACE = 0x04,
                  DT_ENDPOINT = 0x05, DT_HID = 0x21;
// endpoint attributes
constexpr uint8_t XFER_CONTROL = 0, XFER_ISOC = 1, XFER_BULK = 2, XFER_INT = 3;
// interface classes used here
constexpr uint8_t CLASS_HID = 0x03, CLASS_MASS_STORAGE = 0x08, CLASS_HUB = 0x09;

struct Setup {                   // the 8-byte SETUP packet
    uint8_t bmRequestType, bRequest;
    uint16_t wValue, wIndex, wLength;
} __attribute__((packed));

struct DeviceDescriptor {
    uint8_t bLength, bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass, bDeviceSubClass, bDeviceProtocol, bMaxPacketSize0;
    uint16_t idVendor, idProduct, bcdDevice;
    uint8_t iManufacturer, iProduct, iSerialNumber, bNumConfigurations;
} __attribute__((packed));

struct ConfigDescriptor {
    uint8_t bLength, bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces, bConfigurationValue, iConfiguration, bmAttributes, bMaxPower;
} __attribute__((packed));

struct InterfaceDescriptor {
    uint8_t bLength, bDescriptorType, bInterfaceNumber, bAlternateSetting, bNumEndpoints;
    uint8_t bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol, iInterface;
} __attribute__((packed));

struct EndpointDescriptor {
    uint8_t bLength, bDescriptorType, bEndpointAddress, bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed));

static_assert(sizeof(Setup) == 8 && sizeof(DeviceDescriptor) == 18 && sizeof(ConfigDescriptor) == 9 &&
              sizeof(InterfaceDescriptor) == 9 && sizeof(EndpointDescriptor) == 7, "chapter 9 sizes");
}
