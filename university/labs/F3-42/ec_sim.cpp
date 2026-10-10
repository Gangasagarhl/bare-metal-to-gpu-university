// ec_sim.cpp - F3-42 Listing 2: follow a battery reading and a key press from the EC
// firmware to the operating system, byte by byte, then read the battery 1,000 times.
#include "ec_model.h"

int main()
{
    ec::Controller controller(42);
    controller.setBattery(31200, 48000);                  // 31.2 Wh of 48.0 Wh
    for (int i = 0; i < 200; ++i) { controller.step(); }  // the EC firmware runs on its own

    ec::HostDriver os(controller, true);
    os.verbose = true;
    const uint16_t remaining = os.read16(ec::kBatRemainingLo);
    os.verbose = false;
    const uint16_t full = os.read16(ec::kBatFullLo);
    std::printf("--- battery: one 16-bit read = two RD_EC transactions ---\n");
    for (const std::string& l : os.log) { std::printf("%s\n", l.c_str()); }
    std::printf("remaining %u mWh of %u mWh -> %u %%\n", remaining, full, remaining * 100u / full);

    std::printf("--- key press: EC firmware path ---\n");
    controller.pressKey(2, 5);
    for (int i = 0; i < 5; ++i) { controller.step(); }
    for (const std::string& l : controller.trace) { std::printf("%s\n", l.c_str()); }
    std::printf("keyboard port now holds %zu byte(s): %u\n", controller.keyboardOutput.size(),
                controller.keyboardOutput.empty() ? 0u : controller.keyboardOutput.front());

    std::printf("--- lid closed: an SCI event, then the OS queries it (QR_EC) ---\n");
    controller.closeLid();
    std::printf("status register: 0x%02x (SCI_EVT set)\n", controller.readStatus());
    while ((controller.readStatus() & ec::kIBF) != 0) { controller.step(); }
    controller.writeCommand(ec::kQR_EC);
    while ((controller.readStatus() & ec::kOBF) == 0) { controller.step(); }
    const uint8_t q = controller.readData();
    std::printf("QR_EC returned 0x%02x: the OS runs the matching AML method (_Q%02X)\n", q, q);

    int wrong = 0;
    for (int i = 0; i < 1000; ++i) {
        if (os.read16(ec::kBatRemainingLo) != 31200) { ++wrong; }
    }
    std::printf("1000 battery reads with the correct handshake: %d wrong\n", wrong);
    return wrong == 0 && q == ec::kQueryLid ? 0 : 1;
}
