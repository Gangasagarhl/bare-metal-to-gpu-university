// descparse.cc - F11-12: a device-descriptor parser, the kind a USB stack runs on
// bytes a device hands it. The shape used here follows a USB configuration
// descriptor at the level this lab needs: a configuration header carrying a
// 16-bit wTotalLength, followed by a run of descriptors, each starting with its
// own length byte (bLength) and a type byte (bDescriptorType). The EXACT USB
// field layouts and type codes come from the USB specification (see the
// chapter's unverified box); what this file teaches -- that every length, index
// and count from a device is untrusted -- does not depend on those details.
//
// The bug (no -DFIXED) trusts wTotalLength and each bLength: it reads a byte
// inside every descriptor and advances by bLength, without checking that the
// descriptor, or the whole configuration, fits in the buffer the device
// actually sent. A short transfer with a large wTotalLength walks off the end.
// The fix bounds every step. Provides parse_config(data,size) returning the
// number of descriptors seen, or -1 on a rejected (malformed) input.
#include "shared.h"
#include <cstdint>
#include <vector>

namespace {
constexpr unsigned kConfigHeader = 4;   // bLength, bDescriptorType, wTotalLength(2)
constexpr unsigned kDescHeader   = 2;   // bLength, bDescriptorType
}  // namespace

int parse_config(const unsigned char* data, unsigned long size)
{
    std::vector<unsigned char> owned(data, data + size);   // exact heap copy for ASan
    const unsigned char* p = owned.data();

    if (size < kConfigHeader) return -1;
    unsigned w_total = static_cast<unsigned>(p[2] | (p[3] << 8));   // wTotalLength, from the device

#ifdef FIXED
    // The configuration must not claim more bytes than were received.
    if (w_total > size) return -1;
    unsigned long limit = w_total;
#else
    unsigned long limit = w_total;        // BUG: trusted, even if it exceeds size
#endif

    int count = 0;
    unsigned long pos = kConfigHeader;
    while (pos + kDescHeader <= limit) {
        unsigned b_length = p[pos];       // bLength, from the device

#ifdef FIXED
        // A descriptor must be at least its header and must fit in the buffer.
        if (b_length < kDescHeader) return -1;
        if (pos + b_length > size) return -1;
#endif
        // Read one meaningful byte inside the descriptor (its type). In the
        // buggy build, pos can already be past the end of `owned`.
        volatile unsigned char type = p[pos + 1];
        (void)type;

        ++count;
        pos += b_length;                   // BUG path: b_length may be 0 (never advances) or huge
#ifndef FIXED
        if (b_length == 0) break;          // avoid a pure infinite loop so the over-read shows
#endif
    }
    return count;
}
