// usbdev.cpp - F3-38 Listing 2: plug the keyboard device into the model host, enumerate it,
// then let the device "type" two letters and the host poll its interrupt endpoint.
#include "usb_stack.h"

int main()
{
    static_assert(sizeof(usb_device_descriptor) == USB_DT_DEVICE_SIZE, "18-byte device descriptor");
    static_assert(sizeof(usb_config_descriptor) == USB_DT_CONFIG_SIZE, "9-byte configuration header");
    static_assert(sizeof(usb_ctrlrequest) == 8, "SETUP packets carry 8 bytes");
    static_assert(sizeof(usb_endpoint_descriptor) == 9 && USB_DT_ENDPOINT_SIZE == 7,
                  "the header's endpoint struct has 2 audio-only bytes; 7 go on the wire");

    KeyboardDevice keyboard(DeviceOptions{});
    ModelHost host(keyboard);
    if (!host.enumerate()) {
        return 1;
    }
    keyboard.typeKey('h');
    keyboard.typeKey('i');
    const std::string typed = host.pollKeys(6);
    std::printf("-- host received the text \"%s\" in %d packets in total --\n", typed.c_str(), host.packets);
    return typed == "hi" ? 0 : 1;
}
