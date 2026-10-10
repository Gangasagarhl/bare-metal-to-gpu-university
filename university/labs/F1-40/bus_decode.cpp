// F1-40 Listing 1: a tiny shared bus with an address decoder.
// The "CPU" issues read and write transactions; the decoder picks the one
// device whose address window contains the address. A made-up memory map.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct Device
{
    std::string name;
    std::uint32_t base;            // first address of the window
    std::uint32_t size;            // number of byte addresses in the window
    std::vector<std::uint8_t> cells;
};

struct Bus
{
    std::vector<Device> devices;
    int cycle = 0;

    Device* decode(std::uint32_t addr)
    {
        for (Device& d : devices) {
            if (addr >= d.base && addr - d.base < d.size) {
                return &d;
            }
        }
        return nullptr;            // nobody answers: a bus error
    }

    void write(std::uint32_t addr, std::uint8_t data)
    {
        ++cycle;
        Device* d = decode(addr);
        if (d == nullptr) {
            std::printf("cycle %2d  WRITE addr=0x%04X data=0x%02X  -> BUS ERROR (no device)\n",
                        cycle, addr, data);
            return;
        }
        d->cells[addr - d->base] = data;
        std::printf("cycle %2d  WRITE addr=0x%04X data=0x%02X  -> %-6s offset 0x%03X\n",
                    cycle, addr, data, d->name.c_str(), addr - d->base);
    }

    int read(std::uint32_t addr)
    {
        ++cycle;
        Device* d = decode(addr);
        if (d == nullptr) {
            std::printf("cycle %2d  READ  addr=0x%04X            -> BUS ERROR (no device)\n",
                        cycle, addr);
            return -1;
        }
        const std::uint8_t v = d->cells[addr - d->base];
        std::printf("cycle %2d  READ  addr=0x%04X data=0x%02X  <- %-6s offset 0x%03X\n",
                    cycle, addr, v, d->name.c_str(), addr - d->base);
        return v;
    }
};

int main()
{
    Bus bus;
    bus.devices.push_back({"RAM", 0x0000, 0x1000, std::vector<std::uint8_t>(0x1000)});
    bus.devices.push_back({"LEDS", 0x2000, 0x0004, std::vector<std::uint8_t>(0x0004)});
    bus.devices.push_back({"TIMER", 0x3000, 0x0010, std::vector<std::uint8_t>(0x0010)});

    std::printf("memory map: RAM 0x0000-0x0FFF, LEDS 0x2000-0x2003, TIMER 0x3000-0x300F\n");
    bus.write(0x0010, 0x2A);       // store a value in RAM
    bus.read(0x0010);              // load it back
    bus.write(0x2000, 0x05);       // same kind of write, but it lands on the LED register
    bus.write(0x3004, 0x64);       // a timer register
    bus.read(0x3004);
    bus.read(0x1800);              // a hole in the map
    std::printf("total bus cycles: %d (one transaction per cycle on this shared bus)\n",
                bus.cycle);
    return 0;
}
