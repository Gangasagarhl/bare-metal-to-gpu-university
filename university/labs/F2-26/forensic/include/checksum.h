#pragma once
#include <string>

// A simple 32-bit rolling checksum of a text.
std::uint32_t checksum(const std::string& text);
