// tpm.h - DR302 F4-13: a TPM 2.0 driver for the TIS (FIFO) interface, and the few TPM 2.0
// commands a measured boot needs. TIS registers: TCG PC Client Platform TPM Profile
// specification; command layouts: TPM 2.0 Library specification, Part 3. Both tier 1,
// written from memory in this build. The run checks them against QEMU's tpm-tis model,
// SeaBIOS (which sends the same PCR_Extend) and mock_tpm.py (see the chapter's warnings).
#pragma once
#include <stdint.h>

namespace tis {                           // locality 0 registers at 0xFED40000
constexpr uintptr_t BASE = 0xFED40000;
constexpr uint32_t ACCESS = 0x00, INT_ENABLE = 0x08, INTF_CAPABILITY = 0x14, STS = 0x18, DATA_FIFO = 0x24,
                   INTERFACE_ID = 0x30, DID_VID = 0xF00, RID = 0xF04;
constexpr uint8_t ACCESS_VALID = 0x80, ACCESS_ACTIVE = 0x20, ACCESS_REQUEST = 0x02;
constexpr uint32_t STS_VALID = 0x80, STS_COMMAND_READY = 0x40, STS_GO = 0x20, STS_DATA_AVAIL = 0x10,
                   STS_EXPECT = 0x08;
}

namespace tpm2 {
constexpr uint16_t ST_NO_SESSIONS = 0x8001, ST_SESSIONS = 0x8002, ALG_SHA256 = 0x000B;
constexpr uint32_t CC_STARTUP = 0x144, CC_GET_RANDOM = 0x17B, CC_PCR_READ = 0x17E, CC_PCR_EXTEND = 0x182;
constexpr uint32_t RS_PW = 0x40000009;    // the password "session" handle (empty password)
}

namespace tpm {
bool probe();                             // a TIS TPM 2.0 answers at locality 0
// Sends one command (big-endian bytes, header included) and reads the response into
// 'resp'. Returns the response length, or 0 on a timeout or interface error.
uint32_t transmit(const uint8_t* cmd, uint32_t len, uint8_t* resp, uint32_t cap);
uint32_t response_code(const uint8_t* resp);
// A name for the response codes this lab meets (TPM 2.0 Library Part 2, "TPM_RC"), else "other".
const char* rc_name(uint32_t rc);
// TPM2_PCR_Read of one SHA-256 PCR. Returns the response code (0 = success).
uint32_t pcr_read(uint32_t index, uint8_t out[32]);
uint32_t pcr_extend(uint32_t index, const uint8_t digest[32]);
uint32_t get_random(uint8_t* out, uint16_t n);
extern bool trace;                        // print every command and response in hex
}
