// usb_stack.h - F3-38 Listing 1: a minimal USB HID keyboard *device* (the firmware side)
// and a model *host* that enumerates it, logging every packet like a protocol analyser.
// Descriptor structures, request codes and descriptor types come from the Linux UAPI header
// <linux/usb/ch9.h> and <linux/hid.h> installed in the build container; their meaning is
// defined by the USB 2.0 Specification, chapter 9, and the HID class definition (pending
// verification, see the chapter's unverified boxes).
#pragma once
#include <linux/hid.h>
#include <linux/usb/ch9.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

using Bytes = std::vector<uint8_t>;

template <typename T>
void appendStruct(Bytes& out, const T& s, size_t n)       // n = bytes on the wire
{
    const auto* p = reinterpret_cast<const uint8_t*>(&s);
    out.insert(out.end(), p, p + n);
}

inline uint16_t le16(uint16_t v) { return v; }             // host and Cortex-M are little-endian

// ------------------------------------------------------------------------------------------
// The device: what the firmware on the microcontroller implements.
// ------------------------------------------------------------------------------------------
struct DeviceOptions {
    uint8_t maxPacket0 = 8;
    bool countClassDescriptors = true;   // the forensic lab sets this to false
};

class KeyboardDevice {
public:
    explicit KeyboardDevice(DeviceOptions o) : opt_(o) { buildDescriptors(); }

    uint8_t address = 0;                 // 0 until SET_ADDRESS has completed
    uint8_t configuration = 0;           // 0 = not configured
    uint8_t maxPacket0() const { return opt_.maxPacket0; }

    // Control endpoint 0: answer one SETUP. Returns false for "STALL" (request not supported).
    bool setup(const usb_ctrlrequest& r, Bytes& in)
    {
        in.clear();
        const uint8_t type = static_cast<uint8_t>(r.wValue >> 8);
        const uint8_t index = static_cast<uint8_t>(r.wValue & 0xFF);
        if (r.bRequestType == (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE) &&
            r.bRequest == USB_REQ_GET_DESCRIPTOR) {
            if (type == USB_DT_DEVICE) { in = device_; }
            else if (type == USB_DT_CONFIG) { in = config_; }
            else if (type == USB_DT_STRING && index < strings_.size()) { in = strings_[index]; }
            else { return false; }
        } else if (r.bRequestType == (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE) &&
                   r.bRequest == USB_REQ_GET_DESCRIPTOR && type == (HID_DT_REPORT & 0xFF)) {
            in = report_;
        } else if (r.bRequestType == (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE) &&
                   r.bRequest == USB_REQ_SET_ADDRESS) {
            pendingAddress_ = static_cast<uint8_t>(r.wValue);   // takes effect after status
        } else if (r.bRequestType == (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE) &&
                   r.bRequest == USB_REQ_SET_CONFIGURATION) {
            configuration = static_cast<uint8_t>(r.wValue);
        } else if (r.bRequestType == (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) &&
                   r.bRequest == HID_REQ_SET_IDLE) {
            // accepted, nothing to do in this model
        } else {
            return false;
        }
        if (in.size() > r.wLength) {
            in.resize(r.wLength);       // never send more than the host asked for
        }
        return true;
    }

    void statusStageDone()
    {
        if (pendingAddress_ != 0) { address = pendingAddress_; pendingAddress_ = 0; }
    }

    // The application side: queue a key press and its release as two 8-byte reports.
    void typeKey(char c)
    {
        const uint8_t usage = static_cast<uint8_t>(0x04 + (c - 'a'));   // 'a' = usage 0x04
        reports_.push_back(Bytes{0, 0, usage, 0, 0, 0, 0, 0});
        reports_.push_back(Bytes(8, 0));
    }

    // Interrupt IN endpoint 1: a report if one is waiting, otherwise NAK (empty).
    bool interruptIn(Bytes& out)
    {
        if (configuration == 0 || reports_.empty()) { return false; }
        out = reports_.front();
        reports_.pop_front();
        return true;
    }

private:
    void buildDescriptors()
    {
        // Boot-keyboard report descriptor (HID 1.11, appendix B.1), 63 bytes.
        report_ = {0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7,
                   0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01,
                   0x75, 0x08, 0x81, 0x01, 0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01,
                   0x29, 0x05, 0x91, 0x02, 0x95, 0x01, 0x75, 0x03, 0x91, 0x01, 0x95, 0x06,
                   0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07, 0x19, 0x00, 0x29, 0x65,
                   0x81, 0x00, 0xC0};

        usb_device_descriptor d{};
        d.bLength = USB_DT_DEVICE_SIZE;
        d.bDescriptorType = USB_DT_DEVICE;
        d.bcdUSB = le16(0x0200);
        d.bDeviceClass = USB_CLASS_PER_INTERFACE;   // the class is given per interface
        d.bMaxPacketSize0 = opt_.maxPacket0;
        d.idVendor = le16(0xFFFF);                  // placeholder: real products need IDs
        d.idProduct = le16(0x0305);                 // (see the unverified box in F3-38)
        d.bcdDevice = le16(0x0100);
        d.iManufacturer = 1;
        d.iProduct = 2;
        d.bNumConfigurations = 1;
        appendStruct(device_, d, USB_DT_DEVICE_SIZE);

        usb_interface_descriptor i{};
        i.bLength = USB_DT_INTERFACE_SIZE;
        i.bDescriptorType = USB_DT_INTERFACE;
        i.bNumEndpoints = 1;
        i.bInterfaceClass = USB_CLASS_HID;
        i.bInterfaceSubClass = USB_INTERFACE_SUBCLASS_BOOT;
        i.bInterfaceProtocol = USB_INTERFACE_PROTOCOL_KEYBOARD;

        // HID class descriptor: bLength, type, bcdHID, country, count, report type, length.
        const Bytes hid = {9, static_cast<uint8_t>(HID_DT_HID), 0x11, 0x01, 0, 1,
                           static_cast<uint8_t>(HID_DT_REPORT),
                           static_cast<uint8_t>(report_.size()), 0};

        usb_endpoint_descriptor e{};
        e.bLength = USB_DT_ENDPOINT_SIZE;           // 7 on the wire (the struct is larger)
        e.bDescriptorType = USB_DT_ENDPOINT;
        e.bEndpointAddress = USB_DIR_IN | 1;        // endpoint 1, IN (device to host)
        e.bmAttributes = USB_ENDPOINT_XFER_INT;
        e.wMaxPacketSize = le16(8);
        e.bInterval = 10;                           // polling interval (frames at full speed)

        usb_config_descriptor c{};
        c.bLength = USB_DT_CONFIG_SIZE;
        c.bDescriptorType = USB_DT_CONFIG;
        const size_t total = USB_DT_CONFIG_SIZE + USB_DT_INTERFACE_SIZE +
                             (opt_.countClassDescriptors ? hid.size() + USB_DT_ENDPOINT_SIZE : 0);
        c.wTotalLength = le16(static_cast<uint16_t>(total));
        c.bNumInterfaces = 1;
        c.bConfigurationValue = 1;
        c.bmAttributes = USB_CONFIG_ATT_ONE;        // bus-powered
        c.bMaxPower = 50;                           // units of 2 mA at USB 2.0: 100 mA

        appendStruct(config_, c, USB_DT_CONFIG_SIZE);
        appendStruct(config_, i, USB_DT_INTERFACE_SIZE);
        config_.insert(config_.end(), hid.begin(), hid.end());
        appendStruct(config_, e, USB_DT_ENDPOINT_SIZE);

        strings_.push_back(Bytes{4, USB_DT_STRING, 0x09, 0x04});   // language list: 0x0409
        strings_.push_back(utf16("OS305 University"));
        strings_.push_back(utf16("F3-38 keyboard"));
    }

    static Bytes utf16(const std::string& s)
    {
        Bytes b{static_cast<uint8_t>(2 + 2 * s.size()), USB_DT_STRING};
        for (char ch : s) { b.push_back(static_cast<uint8_t>(ch)); b.push_back(0); }
        return b;
    }

    DeviceOptions opt_;
    Bytes device_, config_, report_;
    std::vector<Bytes> strings_;
    std::deque<Bytes> reports_;
    uint8_t pendingAddress_ = 0;
};

// ------------------------------------------------------------------------------------------
// The host: a model of what a PC's USB stack does with a new device, as a packet log.
// ------------------------------------------------------------------------------------------
class ModelHost {
public:
    explicit ModelHost(KeyboardDevice& d) : dev_(d) {}
    int packets = 0;

    // One control transfer: SETUP stage, optional IN data stage split into packets, status.
    bool control(uint8_t rt, uint8_t req, uint16_t value, uint16_t index, uint16_t length,
                 const char* what, Bytes& data)
    {
        usb_ctrlrequest r{};
        r.bRequestType = rt; r.bRequest = req;
        r.wValue = le16(value); r.wIndex = le16(index); r.wLength = le16(length);
        Bytes raw;
        appendStruct(raw, r, sizeof r);
        log("SETUP", 0, "DATA0", raw, what);
        if (!dev_.setup(r, data)) {
            std::printf("        <- STALL (request not supported)\n");
            ++packets;
            return false;
        }
        if ((rt & USB_DIR_IN) != 0) {                     // data stage, device to host
            int toggle = 1;                                // the data stage starts with DATA1
            for (size_t off = 0; off < data.size(); off += maxPacket_) {
                const size_t end = std::min(off + maxPacket_, data.size());
                const Bytes chunk(data.begin() + static_cast<long>(off),
                                  data.begin() + static_cast<long>(end));
                log("IN", 0, toggle ? "DATA1" : "DATA0", chunk, "");
                toggle ^= 1;
            }
        }
        log((rt & USB_DIR_IN) ? "OUT" : "IN", 0, "DATA1", Bytes{}, "status stage (zero-length)");
        dev_.statusStageDone();
        return true;
    }

    bool enumerate()
    {
        Bytes b;
        const uint8_t in = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
        const uint8_t out = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
        std::printf("-- bus reset; device answers at address 0, endpoint 0 --\n");
        maxPacket_ = 8;   // until the host knows better, it reads in 8-byte packets
        control(in, USB_REQ_GET_DESCRIPTOR, USB_DT_DEVICE << 8, 0, 8, "GET_DESCRIPTOR(DEVICE), first 8 bytes", b);
        if (b.size() < 8) { return fail("short device descriptor"); }
        maxPacket_ = b[7];
        std::printf("-- bMaxPacketSize0 = %u; now give the device address 5 --\n", maxPacket_);
        control(out, USB_REQ_SET_ADDRESS, 5, 0, 0, "SET_ADDRESS(5)", b);
        addr_ = dev_.address;
        control(in, USB_REQ_GET_DESCRIPTOR, USB_DT_DEVICE << 8, 0, USB_DT_DEVICE_SIZE, "GET_DESCRIPTOR(DEVICE)", b);
        const unsigned vid = b[8] | (b[9] << 8), pid = b[10] | (b[11] << 8);
        control(in, USB_REQ_GET_DESCRIPTOR, USB_DT_CONFIG << 8, 0, USB_DT_CONFIG_SIZE, "GET_DESCRIPTOR(CONFIG), header", b);
        const uint16_t total = static_cast<uint16_t>(b[2] | (b[3] << 8));
        control(in, USB_REQ_GET_DESCRIPTOR, USB_DT_CONFIG << 8, 0, total, "GET_DESCRIPTOR(CONFIG), wTotalLength bytes", b);
        const Bytes cfg = b;
        control(in, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) | 2, 0x0409, 255, "GET_DESCRIPTOR(STRING 2)", b);
        std::string product;
        for (size_t k = 2; k + 1 < b.size(); k += 2) { product += static_cast<char>(b[k]); }

        // Walk the configuration: every descriptor starts with bLength, bDescriptorType.
        int endpoints = 0, hidDescriptors = 0, interfaces = 0;
        uint8_t epIn = 0, hidClass = 0;
        for (size_t k = 0; k + 1 < cfg.size() && cfg[k] != 0; k += cfg[k]) {
            if (cfg[k + 1] == USB_DT_INTERFACE && k + 6 < cfg.size()) { ++interfaces; hidClass = cfg[k + 5]; }
            if (cfg[k + 1] == (HID_DT_HID & 0xFF)) { ++hidDescriptors; }
            if (cfg[k + 1] == USB_DT_ENDPOINT && k + 2 < cfg.size()) { ++endpoints; epIn = cfg[k + 2]; }
        }
        std::printf("-- device %04x:%04x \"%s\": %d interface(s), class %u, %d HID descriptor(s), %d endpoint(s) --\n",
                    vid, pid, product.c_str(), interfaces, hidClass, hidDescriptors, endpoints);
        if (hidClass != USB_CLASS_HID || hidDescriptors != 1 || endpoints != 1 || (epIn & USB_DIR_IN) == 0) {
            return fail("configuration descriptor does not describe a usable HID interface");
        }
        control(out, USB_REQ_SET_CONFIGURATION, 1, 0, 0, "SET_CONFIGURATION(1)", b);
        control(USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, HID_REQ_SET_IDLE, 0, 0, 0, "HID SET_IDLE(0)", b);
        control(USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE, USB_REQ_GET_DESCRIPTOR,
                (HID_DT_REPORT & 0xFF) << 8, 0, 63, "GET_DESCRIPTOR(HID REPORT)", b);
        std::printf("-- configured: polling interrupt endpoint 0x%02x --\n", epIn);
        return true;
    }

    std::string pollKeys(int polls)
    {
        std::string typed;
        int toggle = 0;
        for (int n = 0; n < polls; ++n) {
            Bytes r;
            if (!dev_.interruptIn(r)) {
                std::printf("IN    addr %u ep1 -> NAK (nothing to report)\n", addr_);
                ++packets;
                continue;
            }
            log("IN", 1, toggle ? "DATA1" : "DATA0", r, r[2] ? "key down" : "all keys up");
            toggle ^= 1;
            if (r[2] >= 0x04 && r[2] <= 0x1D) { typed += static_cast<char>('a' + (r[2] - 0x04)); }
        }
        return typed;
    }

private:
    bool fail(const char* why)
    {
        std::printf("-- ENUMERATION FAILED: %s --\n", why);
        return false;
    }

    void log(const char* pid, int ep, const char* toggle, const Bytes& data, const char* note)
    {
        ++packets;
        std::printf("%-5s addr %u ep%d %s %2zu bytes:", pid, addr_, ep, toggle, data.size());
        for (uint8_t v : data) { std::printf(" %02x", v); }
        if (note[0] != '\0') { std::printf("   %s", note); }
        std::printf("\n");
    }

    KeyboardDevice& dev_;
    uint8_t addr_ = 0;
    uint8_t maxPacket_ = 8;
};
