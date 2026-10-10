// rtc.h - DR301 F4-04: the CMOS real-time clock driver.
#pragma once
#include "rtc_decode.h"

namespace rtc {
void init(uint8_t century_register);        // 0 = no century register (from the FADT)
RtcRaw read_raw();                          // a consistent snapshot (see rtc.cc)
DateTime now();
uint32_t retries();                         // snapshots thrown away because they changed
uint8_t read_register(uint8_t reg);
}
