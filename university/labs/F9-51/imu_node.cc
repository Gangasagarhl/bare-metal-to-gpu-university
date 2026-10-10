// imu_node.cc - F9-51 Listing 1: microcontroller firmware that samples an IMU and publishes
// the samples as framed messages over the UART, on uRTOS (copied unchanged from F3-39).
// It shows the shape of a micro-ROS-style client WITHOUT micro-ROS (not in the build
// container): static memory only, a high-priority sampling task, a lower-priority
// publishing task, a lock-free ring between them, and a framed, checksummed serial link.
// The frame format ("uframe") is this course's own, not a micro-ROS or DDS format.
// The IMU is simulated in software with deterministic values.
#include <stdint.h>

#include "board.h"
#include "urtos.h"

#ifndef RING_SIZE
#define RING_SIZE 16          // slots in the sample ring (one is always left empty)
#endif
#ifndef PUB_PERIOD
#define PUB_PERIOD 10         // ticks between two runs of the publishing task
#endif

namespace {

constexpr uint32_t kTickReload = 24999;      // a tick every 25,000 processor clocks
constexpr uint32_t kStopTick = 122;
constexpr uint32_t kImuPeriod = 2;           // one sample every 2 ticks
constexpr uint8_t kTopicImu = 1;

struct ImuSample {
    uint16_t seq;
    uint32_t tick;
    int16_t accel[3];   // milli-g
    int16_t gyro[3];    // hundredths of a degree per second
};

// ---- single-producer single-consumer ring in static memory -------------------------------
ImuSample ring[RING_SIZE];
uint32_t head = 0;    // written only by the sampling task
uint32_t tail = 0;    // written only by the publishing task
uint32_t dropped = 0, produced = 0, published = 0, maxFill = 0;

bool ringPush(const ImuSample& s)
{
    const uint32_t h = __atomic_load_n(&head, __ATOMIC_RELAXED);
    const uint32_t next = (h + 1) % RING_SIZE;
    if (next == __atomic_load_n(&tail, __ATOMIC_ACQUIRE)) { return false; }
    ring[h] = s;
    __atomic_store_n(&head, next, __ATOMIC_RELEASE);
    const uint32_t fill = (next + RING_SIZE - tail) % RING_SIZE;
    if (fill > maxFill) { maxFill = fill; }
    return true;
}

bool ringPop(ImuSample& s)
{
    const uint32_t t = __atomic_load_n(&tail, __ATOMIC_RELAXED);
    if (t == __atomic_load_n(&head, __ATOMIC_ACQUIRE)) { return false; }
    s = ring[t];
    __atomic_store_n(&tail, (t + 1) % RING_SIZE, __ATOMIC_RELEASE);
    return true;
}

// ---- the simulated sensor ------------------------------------------------------------------
ImuSample readImu(uint16_t seq, uint32_t tick)
{
    const int32_t phase = static_cast<int32_t>(seq % 40);           // a triangle wave
    const int32_t gz = (phase < 20 ? phase : 40 - phase) * 100 - 1000;
    return ImuSample{seq, tick, {0, 0, 1000}, {0, 0, static_cast<int16_t>(gz)}};
}

// ---- framing: 0x7E, length, payload, CRC-16 (polynomial 0x1021, initial 0xFFFF) ------------
uint16_t crc16(const uint8_t* p, uint32_t n)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < n; ++i) {
        crc = static_cast<uint16_t>(crc ^ (p[i] << 8));
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 0x8000) != 0 ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                                      : static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

void put16(uint8_t*& p, uint16_t v) { *p++ = v & 0xFF; *p++ = v >> 8; }   // little-endian
void put32(uint8_t*& p, uint32_t v) { put16(p, v & 0xFFFF); put16(p, v >> 16); }

void sendFrame(const ImuSample& s)
{
    uint8_t frame[2 + 19 + 2];
    uint8_t* p = frame;
    *p++ = 0x7E;
    *p++ = 19;                                   // payload length
    *p++ = kTopicImu;
    put16(p, s.seq);
    put32(p, s.tick);
    for (int16_t v : s.accel) { put16(p, static_cast<uint16_t>(v)); }
    for (int16_t v : s.gyro) { put16(p, static_cast<uint16_t>(v)); }
    put16(p, crc16(frame + 1, 1 + 19));          // covers length and payload
    board::print("F ");                           // the link carries hex text (readable logs)
    for (uint8_t b : frame) {
        board::putChar("0123456789abcdef"[b >> 4]);
        board::putChar("0123456789abcdef"[b & 0xF]);
    }
    board::putChar('\n');
}

// ---- tasks -----------------------------------------------------------------------------------
void imuTask()                                   // priority 3: sample on time, never block
{
    uint32_t release = 0;
    uint16_t seq = 0;
    for (;;) {
        const ImuSample s = readImu(seq++, os::tick());
        produced = produced + 1;
        if (!ringPush(s)) { dropped = dropped + 1; }     // full: count it, never wait
        release += kImuPeriod;
        os::sleepUntil(release);
    }
}

void publishTask()                               // priority 2: drain the ring, send frames
{
    uint32_t release = 0;
    for (;;) {
        ImuSample s;                             // filled by ringPop before any use
        while (ringPop(s)) {
            sendFrame(s);
            published = published + 1;
        }
        release += PUB_PERIOD;
        os::sleepUntil(release);
    }
}

uint32_t imuStack[256], pubStack[256];

void report()
{
    board::print("R produced "); board::printDec(produced);
    board::print(" published "); board::printDec(published);
    board::print(" dropped "); board::printDec(dropped);
    board::print(" ring slots "); board::printDec(RING_SIZE);
    board::print(" max fill "); board::printDec(maxFill);
    board::print(" publish period "); board::printDec(PUB_PERIOD);
    board::print(" ticks\n");
    board::exitEmulator(true);
}

}  // namespace

int main()
{
    board::uartInit();
    board::print("H imu_node: uRTOS, imu every 2 ticks (prio 3), publisher (prio 2)\n");
    const uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    board::print("H crc16(\"123456789\") = "); board::printHex(crc16(check, 9)); board::print("\n");
    os::createTask(imuTask, 3, imuStack, 256, "imu", 'I');
    os::createTask(publishTask, 2, pubStack, 256, "publish", 'P');
    os::start(kTickReload, kStopTick, report);
}
