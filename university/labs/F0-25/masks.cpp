// Eight room lights in one byte: bit 0 is the kitchen, bit 7 is the garden.
#include <bitset>
#include <cstdint>
#include <iostream>

void show(const char* what, std::uint8_t lights)
{
    std::cout << std::bitset<8>(lights) << "  " << what << '\n';
}

int main()
{
    std::uint8_t lights = 0b0000'0000;
    show("all off", lights);

    lights = lights | (1u << 3);            // set bit 3: turn ON the bathroom light
    show("set bit 3 (OR)", lights);

    lights = lights | 0b1000'0001;          // set bits 7 and 0 at once
    show("set bits 7 and 0 (OR)", lights);

    lights = lights & ~(1u << 3);           // clear bit 3: turn OFF the bathroom light
    show("clear bit 3 (AND with NOT)", lights);

    lights = lights ^ 0b0000'0011;          // toggle bits 1 and 0
    show("toggle bits 1 and 0 (XOR)", lights);

    const bool gardenOn = (lights & (1u << 7)) != 0;  // test bit 7
    std::cout << "garden light on? " << (gardenOn ? "yes" : "no") << "\n\n";

    const std::uint8_t status = 0b1011'0110;
    show("status byte", status);
    show("shifted right by 4", static_cast<std::uint8_t>(status >> 4));
    show("low four bits (AND 0x0F)", static_cast<std::uint8_t>(status & 0x0F));
    std::cout << "bits 6..4 as a number: " << ((status >> 4) & 0b111) << '\n';
    std::cout << "1 shifted left by 20 = " << (1u << 20) << '\n';
    return 0;
}
