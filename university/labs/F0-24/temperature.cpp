// Evidence program for the forensic lab "Why is my number negative?".
// The oven sends one byte per reading: the temperature in degrees Celsius, 0 to 250.
#include <cstdint>
#include <iostream>
#include <vector>

int main()
{
    const std::vector<std::uint8_t> packet = {20, 65, 110, 127, 128, 150, 180, 200, 250};
    for (std::uint8_t byte : packet) {
        std::int8_t temperature = static_cast<std::int8_t>(byte);
        std::cout << "oven temperature: " << static_cast<int>(temperature) << " C\n";
    }
    return 0;
}
