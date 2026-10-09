// A 4-input look-up table (LUT) in software: 16 stored bits, and the 4 inputs pick one.
// "Configuring" the LUT means choosing the 16 bits; the same part then becomes any
// 4-input logic function.
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

class Lut4
{
public:
    explicit Lut4(std::uint16_t config) : config_(config) {}

    // The inputs form a 4-bit address: d c b a, with a as the lowest bit.
    int eval(int a, int b, int c, int d) const
    {
        const int address = (d << 3) | (c << 2) | (b << 1) | a;
        return (config_ >> address) & 1;
    }

private:
    std::uint16_t config_;
};

struct Function
{
    std::string name;
    std::function<int(int, int, int, int)> f;
};

// Build the 16 configuration bits by asking the function for every input pattern.
std::uint16_t configure(const Function& fn)
{
    std::uint16_t bits = 0;
    for (int address = 0; address < 16; ++address) {
        const int a = address & 1;
        const int b = (address >> 1) & 1;
        const int c = (address >> 2) & 1;
        const int d = (address >> 3) & 1;
        if (fn.f(a, b, c, d) != 0) {
            bits = static_cast<std::uint16_t>(bits | (1u << address));
        }
    }
    return bits;
}

int main()
{
    const std::vector<Function> functions = {
        {"AND of 4 inputs", [](int a, int b, int c, int d) { return a & b & c & d; }},
        {"full-adder sum (a, b, carry-in c)", [](int a, int b, int c, int) { return a ^ b ^ c; }},
        {"full-adder carry (a, b, carry-in c)",
         [](int a, int b, int c, int) { return (a & b) | (a & c) | (b & c); }},
        {"bit 3 of count + 1 (count = d c b a)",
         [](int a, int b, int c, int d) { return (((d << 3) | (c << 2) | (b << 1) | a) + 1) >> 3 & 1; }},
    };
    int errors = 0;
    for (const Function& fn : functions) {
        const std::uint16_t config = configure(fn);
        const Lut4 lut(config);
        int wrong = 0;
        for (int address = 0; address < 16; ++address) {
            const int a = address & 1;
            const int b = (address >> 1) & 1;
            const int c = (address >> 2) & 1;
            const int d = (address >> 3) & 1;
            wrong += (lut.eval(a, b, c, d) != fn.f(a, b, c, d));
        }
        std::printf("%-38s config = 0x%04X  checked 16 inputs, %d wrong\n", fn.name.c_str(),
                    config, wrong);
        errors += wrong;
    }
    std::printf("%s\n", errors == 0 ? "RESULT: PASS" : "RESULT: FAIL");
    return errors == 0 ? 0 : 1;
}
