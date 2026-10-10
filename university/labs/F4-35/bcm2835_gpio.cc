// bcm2835_gpio.cc - F4-35: a pin controller. On the BCM2835 family each pin has a 3-bit
// function field: GPFSELn registers at offset 4 * (pin / 10), field at bit 3 * (pin % 10)
// ("BCM2835 ARM Peripherals", GPIO chapter - title only, pending verification). Other drivers
// ask for a group of pins through dm::pins_set().
#include "dm.h"

namespace {
uint64_t g_base = 0;

bool set_function(int, uint32_t first, uint32_t count, uint32_t function)
{
    if (function > 7 || first + count > 54) {
        return false;
    }
    for (uint32_t pin = first; pin < first + count; ++pin) {
        uint64_t reg = g_base + 4 * (pin / 10);
        uint32_t shift = 3 * (pin % 10);
        uint32_t v = k::rd32(reg);
        v = (v & ~(7u << shift)) | (function << shift);
        k::wr32(reg, v);
    }
    k::printf("  pins %u-%u set to function %u (GPFSEL4 now 0x%08x, GPFSEL5 0x%08x)\n", first, first + count - 1,
              function, k::rd32(g_base + 0x10), k::rd32(g_base + 0x14));
    return true;
}

dm::Probe bcm2835_gpio_probe(dm::Device& dev)
{
    g_base = dev.base;
    dm::pin_provider(dev.node, set_function);
    k::printf("  gpio %s: pin controller registered (GPFSEL4 0x%08x, GPFSEL5 0x%08x)\n", dm::path(dev),
              k::rd32(g_base + 0x10), k::rd32(g_base + 0x14));
    return dm::Probe::kOk;
}
DR403_DRIVER(bcm2835gpio, "bcm2835-gpio", bcm2835_gpio_probe, "brcm,bcm2835-gpio");
}  // namespace
