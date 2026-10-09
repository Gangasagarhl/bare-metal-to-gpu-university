// F1-46 Listing 1: decode the device descriptor and the configuration
// descriptor that a real (emulated) USB keyboard sent to the firmware.
// Input: four lines of hex bytes, extracted from QEMU's capture by run.sh:
// the SETUP packet and the reply for the device descriptor, then the same
// for the configuration descriptor set.
// Field layout and constants: the Linux UAPI header linux/usb/ch9.h.
#include <linux/usb/ch9.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::uint8_t> read_hex_line()
{
    std::string line;
    std::getline(std::cin, line);
    std::istringstream in(line);
    std::vector<std::uint8_t> v;
    unsigned x = 0;
    while (in >> std::hex >> x) {
        v.push_back(static_cast<std::uint8_t>(x));
    }
    return v;
}

unsigned le16(const std::uint8_t* p)
{
    return static_cast<unsigned>(p[0] | (p[1] << 8));       // USB fields are little-endian
}

void print_setup(const std::vector<std::uint8_t>& s)
{
    usb_ctrlrequest r{};
    std::memcpy(&r, s.data(), sizeof r);
    std::printf("SETUP: bmRequestType 0x%02x (%s), bRequest %u%s, descriptor type %u, "
                "wLength %u\n",
                r.bRequestType, (r.bRequestType & USB_DIR_IN) ? "device-to-host" : "host-to-device",
                r.bRequest, r.bRequest == USB_REQ_GET_DESCRIPTOR ? " (GET_DESCRIPTOR)" : "",
                s[3], le16(s.data() + 6));
}

int main()
{
    const std::vector<std::uint8_t> dev_setup = read_hex_line();
    const std::vector<std::uint8_t> dev = read_hex_line();
    const std::vector<std::uint8_t> cfg_setup = read_hex_line();
    const std::vector<std::uint8_t> cfg = read_hex_line();
    if (dev_setup.size() != 8 || cfg_setup.size() != 8 || dev.size() < 8 || dev[1] != USB_DT_DEVICE) {
        std::printf("unexpected input\n");
        return 1;
    }
    print_setup(dev_setup);
    std::printf("device descriptor: %zu of its %d bytes were sent (bLength says %u)\n",
                dev.size(), USB_DT_DEVICE_SIZE, dev[0]);
    std::printf("  bcdUSB 0x%04x  bDeviceClass %u  bMaxPacketSize0 %u\n", le16(dev.data() + 2),
                dev[4], dev[7]);
    if (dev.size() >= USB_DT_DEVICE_SIZE) {
        usb_device_descriptor d{};
        std::memcpy(&d, dev.data(), USB_DT_DEVICE_SIZE);
        std::printf("  idVendor 0x%04x  idProduct 0x%04x  configurations %u\n",
                    le16(dev.data() + 8), le16(dev.data() + 10), d.bNumConfigurations);
    }
    print_setup(cfg_setup);
    std::printf("configuration descriptor set (%zu bytes)\n", cfg.size());
    for (std::size_t p = 0; p + 2 <= cfg.size() && cfg[p] >= 2; p += cfg[p]) {
        const std::uint8_t len = cfg[p];
        const std::uint8_t type = cfg[p + 1];
        if (type == USB_DT_CONFIG) {
            std::printf("  [%2zu] CONFIGURATION  total length %u, interfaces %u\n", p,
                        le16(&cfg[p + 2]), cfg[p + 4]);
        } else if (type == USB_DT_INTERFACE) {
            std::printf("  [%2zu] INTERFACE      number %u, class 0x%02x subclass 0x%02x "
                        "protocol 0x%02x, endpoints %u\n",
                        p, cfg[p + 2], cfg[p + 5], cfg[p + 6], cfg[p + 7], cfg[p + 4]);
        } else if (type == USB_DT_ENDPOINT && len >= USB_DT_ENDPOINT_SIZE) {
            usb_endpoint_descriptor e{};
            std::memcpy(&e, &cfg[p], USB_DT_ENDPOINT_SIZE);
            const bool in = (e.bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN;
            const int xfer = e.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
            std::printf("  [%2zu] ENDPOINT       number %u %s, type %s, max packet %u, "
                        "bInterval %u\n",
                        p, e.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK, in ? "IN" : "OUT",
                        xfer == USB_ENDPOINT_XFER_INT ? "interrupt" : "other",
                        le16(&cfg[p + 4]), e.bInterval);
        } else {
            std::printf("  [%2zu] type 0x%02x, %u bytes (class-specific)\n", p, type, len);
        }
    }
    return 0;
}
