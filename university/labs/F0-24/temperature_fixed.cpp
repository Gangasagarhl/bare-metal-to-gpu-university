// Fixed version for the forensic lab answer key.
// The oven sends one byte per reading: the temperature in degrees Celsius, 0 to 250.
#include <cstdint>
#include <iostream>
#include <vector>

int main()
{
    const std::vector<std::uint8_t> packet = {20, 65, 110, 127, 128, 150, 180, 200, 250};
    for (std::uint8_t byte : packet) {
        int temperature = byte;  // fixed: keep the reading unsigned (0 to 255)
        std::cout << "oven temperature: " << temperature << " C\n";
    }
    return 0;
}
