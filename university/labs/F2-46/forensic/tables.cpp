// tables.cpp: a 16 KiB lookup table that a teammate added for a faster checksum.
#include <stdint.h>

extern const uint8_t g_crc_table[16384];
const uint8_t g_crc_table[16384] = {0x00, 0x07, 0x0e, 0x09};
