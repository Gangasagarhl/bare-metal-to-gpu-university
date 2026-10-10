// hid_parse.h - a HID report descriptor parser: which bits of a report mean what.
// Short items only (long items are skipped): prefix byte = tag (bits 7..4), type (bits 3..2:
// 0 main, 1 global, 2 local), size (bits 1..0: 0, 1, 2 or 4 data bytes). Item tags were written
// from memory of the Device Class Definition for HID 1.11 (title only in this build); the lab
// checks them by parsing descriptors that a working program (QEMU) sends, and requiring every
// report to come out a whole number of bytes.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace hid {

struct Field {
    uint8_t report_id;          // 0: the descriptor uses no report IDs
    uint16_t usage_page;
    uint32_t usage;             // first usage (or usage minimum)
    uint32_t usage_max;         // == usage when a single usage
    int32_t logical_min, logical_max;
    uint32_t bit_offset;        // inside the report, after the report-ID byte if there is one
    uint32_t size, count;       // bits per value, number of values
    uint32_t flags;             // Input item data: bit 0 constant, bit 1 variable, bit 2 relative
    int depth;                  // collection depth
};

struct Report { uint8_t id; uint32_t bits; };

struct Parsed {
    std::vector<Field> fields;
    std::vector<Report> reports;   // input reports, with their total length in bits
    std::vector<std::string> collections;
    std::vector<std::string> items;   // "offset: bytes  type tag" for each short item
    std::vector<std::string> warnings;
    std::string error;             // empty when the descriptor parsed cleanly
};

Parsed parse(const std::vector<uint8_t>& d);
std::string usage_name(uint16_t page, uint32_t usage);
// Extract one field's value from a report (bytes after the report ID, if any).
int32_t extract(const std::vector<uint8_t>& report, const Field& f, uint32_t index);

} // namespace hid
