// app_race.cc - app.cc with the first version of the UART handler (clears the flag last).
//   * timer 0 interrupts 100 times per 1,000,000 timer clocks (a "tick" every 10,000 clocks);
//   * UART 0 receive interrupts fill a ring buffer; main() reads commands from it;
//   * an I2C driver (Listing 1) talks to the temperature sensor QEMU attached at 0x48.
// IRQ numbers 8 (timer 0) and 0 (UART 0 receive) were measured by irqmap.cc in this build.
#include "board.h"
#include "i2c_bitbang.h"

namespace {

using board::reg;

// ---- interrupt-safe ring buffer: one producer (the IRQ handler), one consumer (main) ----
template <uint32_t N>
class RingBuffer {
    static_assert((N & (N - 1)) == 0, "N must be a power of two");
public:
    bool push(uint8_t b)               // called only from the interrupt handler
    {
        const uint32_t h = head_;
        if (h - tail_ == N) {
            return false;              // full: drop the byte (and count it)
        }
        buf_[h % N] = b;
        asm volatile("" ::: "memory"); // compiler barrier: the byte is stored before the index
        head_ = h + 1;                 // publish after the data is written
        return true;
    }
    bool pop(uint8_t& b)               // called only from main()
    {
        const uint32_t t = tail_;
        if (t == head_) {
            return false;
        }
        asm volatile("" ::: "memory"); // compiler barrier: read the byte only after the index
        b = buf_[t % N];
        asm volatile("" ::: "memory"); // ... and free the slot only after reading it
        tail_ = t + 1;
        return true;
    }
private:
    uint8_t buf_[N] = {};
    volatile uint32_t head_ = 0;
    volatile uint32_t tail_ = 0;
};

RingBuffer<16> rx;
volatile uint32_t ticks = 0;
volatile uint32_t rxDropped = 0;

constexpr uintptr_t kTimer0 = 0x40000000;   // CMSDK APB timer: CTRL 0x0, VALUE 0x4, RELOAD 0x8, INT 0xC
constexpr uint32_t kTimer0Irq = 8;
constexpr uint32_t kUart0RxIrq = 0;
constexpr uintptr_t kNvicIser0 = 0xE000E100;

// ---- the I2C pins: SBCon two-wire interface, bit 0 = SCL, bit 1 = SDA ----
struct SbconPins {
    static constexpr uintptr_t kBase = 0x4002A000;   // the bus QEMU attached the sensor to
    void scl(bool high) { reg(kBase + (high ? 0x0 : 0x4)) = 1u; }   // 0x0 sets, 0x4 clears
    void sda(bool high) { reg(kBase + (high ? 0x0 : 0x4)) = 2u; }
    bool readSda() { return (reg(kBase) & 2u) != 0; }
    void wait() {}                     // the emulator needs no delay; real hardware does
};

SbconPins pins;
I2cMaster<SbconPins> i2c(pins);
constexpr uint8_t kSensorAddr = 0x48;

// Temperature register: 16 bits, MSB first, two's complement, 1/256 degC per bit after
// sign extension (the sensor fills the top 12 bits). Returns hundredths of a degree.
bool readTemperature(int32_t& centi)
{
    const uint8_t pointer = 0x00;      // register 0 = temperature
    uint8_t raw[2] = {0, 0};
    if (!i2c.transfer(kSensorAddr, &pointer, 1, raw, 2)) {
        return false;
    }
    const int16_t value = static_cast<int16_t>((raw[0] << 8) | raw[1]);
    centi = (static_cast<int32_t>(value) * 100) / 256;
    return true;
}

void printCenti(int32_t c)
{
    if (c < 0) {
        board::putChar('-');
        c = -c;
    }
    board::printDec(static_cast<uint32_t>(c / 100));
    board::putChar('.');
    board::putChar(static_cast<char>('0' + (c / 10) % 10));
    board::putChar(static_cast<char>('0' + c % 10));
}

}  // namespace

extern "C" void Irq8_Handler()       // timer 0
{
    reg(kTimer0 + 0x0C) = 1;         // INTCLEAR: acknowledge, or the interrupt fires again
    ticks = ticks + 1;
}

extern "C" void Irq0_Handler()       // UART 0 receive
{
    while ((board::uart0().state & 0x2u) != 0) {          // RX buffer full: a byte waits
        if (!rx.push(static_cast<uint8_t>(board::uart0().data))) {
            rxDropped = rxDropped + 1;
        }
    }
    board::uart0().intstatus = 0x2;                       // clear the RX interrupt (too late)
}

int main()
{
    board::uartInit();
    board::print("F3-37: timer + UART interrupts, I2C sensor driver\n");

    reg(kTimer0 + 0x08) = 9999;                 // RELOAD: a tick every 10,000 timer clocks
    reg(kTimer0 + 0x04) = 9999;                 // VALUE
    reg(kTimer0 + 0x0C) = 1;                    // clear an old flag
    reg(kTimer0 + 0x00) = (1u << 3) | 1u;       // interrupt enable, timer enable
    board::uart0().ctrl = 0x1 | 0x2 | (1u << 3);  // TX on, RX on, RX interrupt on
    reg(kNvicIser0) = (1u << kTimer0Irq) | (1u << kUart0RxIrq);

    for (;;) {
        uint8_t c = 0;
        if (!rx.pop(c)) {
            asm volatile("wfi");                // sleep until the next interrupt
            continue;
        }
        if (c == 's') {                         // scan the bus
            board::print("scan:");
            for (uint8_t a = 0x08; a < 0x78; ++a) {
                if (i2c.probe(a)) {
                    board::print(" "); board::printHex(a);
                }
            }
            board::print("\n");
        } else if (c == 'r') {                  // read the temperature
            int32_t centi = 0;
            if (readTemperature(centi)) {
                board::print("temperature = "); printCenti(centi); board::print(" C\n");
            } else {
                board::print("temperature: no ACK from sensor\n");
            }
        } else if (c == 'c') {                  // configuration register: 12-bit resolution
            const uint8_t cfg[2] = {0x01, 0x60};  // pointer 1, then the new value
            board::print(i2c.transfer(kSensorAddr, cfg, 2, nullptr, 0) ? "config: 12-bit\n"
                                                                       : "config: no ACK\n");
        } else if (c == 'w') {                  // wait 10 ticks, sleeping between interrupts
            const uint32_t until = ticks + 10;
            while (ticks < until) {
                asm volatile("wfi");
            }
            board::print("waited 10 ticks\n");
        } else if (c == 't') {                  // report the tick count
            board::print("ticks so far = "); board::printDec(ticks); board::print("\n");
        } else if (c == 'q') {
            board::print("bye (dropped bytes: "); board::printDec(rxDropped); board::print(")\n");
            board::exitEmulator(true);
        }
    }
}
