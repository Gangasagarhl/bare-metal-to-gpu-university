// F1-41 forensic evidence: "works in debug, fails in release".
// A helper thread stands in for the hardware: 50 ms after start it puts a byte
// in DATA and then sets bit 0 of STATUS. The "driver" waits for bit 0 through
// a plain (non-volatile) pointer, then reads DATA. Built at -O0 and at -O2.
// (In C++ terms the helper thread makes this a data race, which is undefined
// behaviour: that is exactly the licence the optimiser uses.)
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

std::uint32_t g_status = 0;   // bit 0: data ready
std::uint32_t g_data = 0;

std::uint32_t read_byte(const std::uint32_t* status, const std::uint32_t* data)
{
    while ((*status & 1u) == 0) {
        // wait for the device
    }
    return *data;
}

int main()
{
    std::thread device([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        volatile std::uint32_t* d = &g_data;
        volatile std::uint32_t* s = &g_status;
        *d = 0x41;            // 'A'
        *s = 1;
    });
    const auto t0 = std::chrono::steady_clock::now();
    const std::uint32_t b = read_byte(&g_status, &g_data);
    const auto waited = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);
    device.join();
    std::printf("driver read 0x%02X after waiting about %lld ms (expected 0x41 after ~50 ms)\n",
                static_cast<unsigned>(b), static_cast<long long>(waited.count()));
    return 0;
}
