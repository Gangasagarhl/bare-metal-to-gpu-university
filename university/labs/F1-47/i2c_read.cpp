// F1-47 Listing 1: read two bytes from register 0x00 of the sensor at 0x48,
// record the bus, print the capture, then decode it from the samples alone.
#include "i2c_sim.h"

int main()
{
    Bus bus;
    Target sensor;
    Controller c(bus, sensor);
    std::vector<std::uint8_t> data;
    const bool ok = read_reg(c, 0x48, 0x00, 2, data);
    std::printf("driver: read_reg(0x48, 0x00, 2) -> %s", ok ? "ok" : "NACK");
    for (std::uint8_t b : data) std::printf(" 0x%02X", b);
    std::printf("\ncapture (%zu samples, 4 per bit):\n", bus.trace.size());
    print_capture(bus.trace);
    std::printf("decoded from the capture:\n");
    decode(bus.trace);
    return 0;
}
