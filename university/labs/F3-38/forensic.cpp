// forensic.cpp - F3-38 forensic evidence: the same device built with the options of the
// release candidate ("v0.9-rc2"). Nothing else changed.
#include "usb_stack.h"

int main()
{
    DeviceOptions rc2;
    rc2.maxPacket0 = 8;
    rc2.countClassDescriptors = false;
    KeyboardDevice keyboard(rc2);
    ModelHost host(keyboard);
    const bool ok = host.enumerate();
    std::printf("-- result: %s --\n", ok ? "keyboard usable" : "device not usable, no driver bound");
    return 0;   // the evidence run itself succeeded; the failure is the subject of the lab
}
