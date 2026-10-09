// fb.h - F3-23: a linear framebuffer from QEMU's standard VGA device (Bochs VBE interface).
#pragma once
#include <cstdint>

namespace fb {

struct Info {
    uint64_t phys;        // BAR0 of the PCI display device: the linear framebuffer
    uint32_t* pixels;     // where the kernel mapped it
    int width, height, pitch_pixels;
};

bool init(int width, int height, Info& out);   // finds the device, sets the mode, maps the memory

} // namespace fb
