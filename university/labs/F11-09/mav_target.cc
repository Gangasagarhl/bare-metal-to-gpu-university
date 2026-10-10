// mav_target.cc - a MAVLink-v1-STYLE frame scanner, the second fuzzing target
// of the F11-09 lab. A ground station reads a byte stream from a radio link and
// must find frames in it. The framing used here (start byte 0xFE, then a length
// byte, then seq/sysid/compid/msgid, then the payload, then a two-byte
// checksum) follows the MAVLink v1 frame layout at the level this lab needs; the
// EXACT checksum algorithm and the per-message "CRC_EXTRA" byte are NOT
// reproduced here and must be taken from the MAVLink documentation (see the
// chapter's unverified box). What this file teaches is the untrusted-length
// bug, which is independent of the checksum details.
// The payload-length field comes from the wire and must never be trusted as a
// count of bytes that are actually present. The bug (no -DFIXED) does exactly
// that. Compiled with -fsanitize-coverage=trace-pc.
#include "fuzz.h"
#include <cstdint>
#include <vector>

namespace {
constexpr unsigned char kStx = 0xFE;   // MAVLink v1 start-of-frame marker
constexpr unsigned kHeader = 6;        // stx,len,seq,sysid,compid,msgid
constexpr unsigned kChecksum = 2;      // ck_a, ck_b

// A toy running checksum over the frame bytes. NOT the real MAVLink CRC-16; it
// only has to touch every payload byte so that an over-long length over-reads.
uint16_t toy_sum(const unsigned char* p, unsigned long from, unsigned long to)
{
    uint16_t s = 0xffff;
    for (unsigned long i = from; i < to; ++i) s = static_cast<uint16_t>(s + p[i] * 31u + i);
    return s;
}
}  // namespace

int fuzz_one(const unsigned char* data, unsigned long size)
{
    std::vector<unsigned char> owned(data, data + size);   // exact heap copy for ASan
    const unsigned char* p = owned.data();

    unsigned long pos = 0;
    unsigned frames = 0;
    while (pos < size) {
        if (p[pos] != kStx) { ++pos; continue; }           // scan for a start byte
        if (size - pos < 2) break;                         // need at least stx + len
        unsigned len = p[pos + 1];                         // UNTRUSTED payload length (0..255)

#ifdef FIXED
        // The fix: before touching the payload or the checksum, require that the
        // whole frame is actually present in the buffer.
        if (size - pos < kHeader + len + kChecksum) break;
#endif
        // Checksum is computed over everything from the length byte to the end
        // of the payload. In the buggy build `len` can claim bytes that are not
        // there, so toy_sum reads past the end of `owned`.
        unsigned long payload_end = pos + kHeader + len;
        uint16_t cs = toy_sum(p, pos + 1, payload_end);

        unsigned char ck_a = p[payload_end];               // and the two checksum bytes
        unsigned char ck_b = p[payload_end + 1];
        volatile bool valid = ((cs & 0xff) == ck_a) && ((cs >> 8) == ck_b);
        (void)valid;

        ++frames;
        pos = payload_end + kChecksum;                     // advance past this frame
    }
    (void)frames;
    return 0;
}
