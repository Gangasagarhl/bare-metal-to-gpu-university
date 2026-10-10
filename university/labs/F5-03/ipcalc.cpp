// ipcalc.cpp - read IPv4 addresses with a prefix length ("a.b.c.d/n") and work out the
// subnet mask, the network address, the broadcast address and the usable host range.
#include <bitset>
#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

std::optional<std::uint32_t> parseAddress(const std::string& text)
{
    std::istringstream in(text);
    std::uint32_t value = 0;
    for (int part = 0; part < 4; ++part) {
        int octet = -1;
        char dot = '.';
        if (part > 0 && !(in >> dot)) { return std::nullopt; }
        if (!(in >> octet) || dot != '.' || octet < 0 || octet > 255) { return std::nullopt; }
        value = (value << 8) | static_cast<std::uint32_t>(octet);
    }
    return value;
}

std::string dotted(std::uint32_t a)
{
    return std::to_string(a >> 24) + '.' + std::to_string((a >> 16) & 0xFF) + '.' +
           std::to_string((a >> 8) & 0xFF) + '.' + std::to_string(a & 0xFF);
}

std::string bits(std::uint32_t a)
{
    const std::string s = std::bitset<32>(a).to_string();
    return s.substr(0, 8) + '.' + s.substr(8, 8) + '.' + s.substr(16, 8) + '.' + s.substr(24, 8);
}

int main()
{
    std::string line;
    while (std::getline(std::cin, line)) {
        const auto slash = line.find('/');
        if (slash == std::string::npos) { continue; }
        const auto addr = parseAddress(line.substr(0, slash));
        const int prefix = std::stoi(line.substr(slash + 1));
        if (!addr || prefix < 0 || prefix > 32) {
            std::cout << line << ": not a valid IPv4 address/prefix\n\n";
            continue;
        }
        // prefix ones followed by (32 - prefix) zeros; shifting by 32 is avoided on purpose
        const std::uint32_t mask = prefix == 0 ? 0u : ~std::uint32_t{0} << (32 - prefix);
        const std::uint32_t network = *addr & mask;
        const std::uint32_t broadcast = network | ~mask;
        std::cout << "input      " << line << '\n'
                  << "address    " << bits(*addr) << "  " << dotted(*addr) << '\n'
                  << "mask       " << bits(mask) << "  " << dotted(mask) << '\n'
                  << "network    " << bits(network) << "  " << dotted(network) << '\n'
                  << "broadcast  " << bits(broadcast) << "  " << dotted(broadcast) << '\n';
        if (prefix <= 30) {
            std::cout << "hosts      " << dotted(network + 1) << " to " << dotted(broadcast - 1)
                      << "  (" << (broadcast - network - 1) << " usable addresses)\n\n";
        } else {
            std::cout << "hosts      /31 and /32 are special cases (see the chapter)\n\n";
        }
    }
    return 0;
}
