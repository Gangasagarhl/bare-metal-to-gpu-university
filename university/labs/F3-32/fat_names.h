// fat_names.h - F3-32: the name rules of FAT32, after the Microsoft Extensible Firmware
// Initiative FAT32 File System Specification (sections on directory entries and long names).
#pragma once
#include <array>
#include <cstdint>
#include <string>

// 8.3 short name as stored: 11 bytes, name padded with spaces to 8, extension to 3.
using ShortName = std::array<std::uint8_t, 11>;

// Checksum of a short name, stored in every long-name entry that belongs to it.
inline std::uint8_t lfn_checksum(const ShortName& sn)
{
    std::uint8_t sum = 0;
    for (std::uint8_t c : sn)
        sum = static_cast<std::uint8_t>(((sum & 1) ? 0x80 : 0) + (sum >> 1) + c);   // rotate right, add
    return sum;
}

// "NAME.EXT" form of a stored short name (spaces removed), as tools display it.
inline std::string short_to_string(const ShortName& sn)
{
    std::string base, ext;
    for (int i = 0; i < 8; ++i) if (sn[i] != ' ') base += static_cast<char>(sn[i]);
    for (int i = 8; i < 11; ++i) if (sn[i] != ' ') ext += static_cast<char>(sn[i]);
    return ext.empty() ? base : base + "." + ext;
}

// The basis of a generated short name: upper case, characters that are not allowed become '_',
// spaces and all periods but the last are dropped; base cut to 8 characters, extension to 3.
// lossy is set when characters were dropped, replaced or cut off (then a "~n" tail is needed);
// recased is set when only upper-casing changed the name (then a long-name entry keeps the case).
inline ShortName short_basis(const std::string& longname, bool& lossy, bool& recased)
{
    ShortName sn;
    sn.fill(' ');
    std::size_t dot = longname.rfind('.');
    if (dot == 0) dot = std::string::npos;          // ".profile" has no extension
    std::string base = longname.substr(0, dot);
    std::string ext = dot == std::string::npos ? "" : longname.substr(dot + 1);
    lossy = false;
    recased = false;
    auto conv = [&](const std::string& in, std::size_t max, std::size_t at) {
        std::size_t n = 0;
        for (char ch : in) {
            unsigned char c = static_cast<unsigned char>(ch);
            if (c == ' ' || c == '.') { lossy = true; continue; }
            if (c >= 'a' && c <= 'z') { c = static_cast<unsigned char>(c - 'a' + 'A'); recased = true; }
            const std::string bad = "\"*+,/:;<=>?[\\]|";
            if (c < 0x20 || c > 0x7E || bad.find(static_cast<char>(c)) != std::string::npos) { c = '_'; lossy = true; }
            if (n == max) { lossy = true; break; }
            sn[at + n++] = c;
        }
    };
    conv(base, 8, 0);
    conv(ext, 3, 8);
    return sn;
}

// Put "~n" into a basis: keep as many base characters as fit in 8 with the tail.
inline ShortName with_tail(ShortName sn, int n)
{
    std::string tail = "~" + std::to_string(n);
    std::size_t len = 0;
    while (len < 8 && sn[len] != ' ') ++len;
    std::size_t keep = std::min(len, 8 - tail.size());
    for (std::size_t i = 0; i < tail.size(); ++i) sn[keep + i] = static_cast<std::uint8_t>(tail[i]);
    for (std::size_t i = keep + tail.size(); i < 8; ++i) sn[i] = ' ';
    return sn;
}
