// F1-47 forensic evidence: the same sensor, a new driver. The board file says
// "sensor address: 0x90 (write) / 0x91 (read)" and the driver author copied 0x90
// into the driver's configuration. The bus API takes a 7-bit address.
#include "i2c_sim.h"

int main()
{
    const unsigned configured_address = 0x90;         // from the driver's config
    Bus bus;
    Target sensor;                                    // still at 0x48
    Controller c(bus, sensor);
    std::vector<std::uint8_t> data;
    const bool ok = read_reg(c, configured_address, 0x00, 2, data);
    std::printf("driver log: sensor read at 0x%02X %s\n", configured_address,
                ok ? "ok" : "FAILED: no ACK from the device");
    std::printf("logic analyser capture (%zu samples, 4 per bit):\n", bus.trace.size());
    print_capture(bus.trace);
    std::printf("protocol decoder:\n");
    decode(bus.trace);
    return 0;
}
