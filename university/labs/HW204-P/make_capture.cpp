// HW204 practical (P), Lab Engineer's generator (NOT handed to candidates).
// Produces the exam capture: a bus with one sensor at 7-bit address 0x5A
// (exercise value) whose registers are 0x6B, 0x00, 0x2C, 0x8F. The driver
// under test does three things: it probes address 0x1E (nothing there), it
// writes 0x80 into the sensor's register 0x01, and it reads two bytes from
// register 0x02. The text after "ground truth" is what the capture encodes;
// the capture rows alone are the candidates' input (i2c_decode_start.in).
#include "i2c_exam_model.h"

int main()
{
    Bus bus;
    Target sensor{0x5A, {0x6B, 0x00, 0x2C, 0x8F}};
    bus.targets.push_back(&sensor);
    Controller c(bus);

    const bool p = probe(c, 0x1E);
    const bool w = write_reg(c, 0x5A, 0x01, 0x80);
    std::vector<std::uint8_t> data;
    const bool r = read_reg(c, 0x5A, 0x02, 2, data);

    std::printf("capture (%zu samples, 4 per bit):\n", bus.trace.size());
    print_capture(bus.trace);
    std::printf("ground truth (the driver's own log, not given to candidates):\n");
    std::printf("  probe(0x1E) -> %s\n", p ? "ACK" : "NACK");
    std::printf("  write_reg(0x5A, 0x01, 0x80) -> %s; register 1 now 0x%02X\n", w ? "ok" : "NACK", sensor.regs[1]);
    std::printf("  read_reg(0x5A, 0x02, 2) -> %s", r ? "ok" : "NACK");
    for (std::uint8_t b : data) std::printf(" 0x%02X", b);
    std::printf("\n");
    return 0;
}
