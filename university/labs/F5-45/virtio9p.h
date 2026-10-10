// virtio9p.h - DS403 kernel, F5-45: a polled legacy virtio transport for QEMU's 9P device.
// One request = one buffer the device reads (our T-message) + one it writes (its R-message).
#pragma once
#include <stdint.h>

bool v9p_init(char* mount_tag, int cap);                 // find 1af4:1009, set up queue 0
int v9p_call(const uint8_t* req, uint32_t len, uint8_t* rep, uint32_t cap);   // reply length
