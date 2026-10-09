// A teaching model of DRAM: every stored 1 is charge on a tiny capacitor that leaks.
// The leak rate, the threshold and the sizes are exercise values for this chapter,
// not data of any real DRAM part.
#include <array>
#include <cstdio>

constexpr int kRows = 8;
constexpr int kBits = 8;
constexpr double kKeepPerTick = 0.95;   // each tick a cell keeps 95 % of its charge
constexpr double kThreshold = 0.5;      // the sense amplifier reads 1 above this

class Dram
{
public:
    void write(int row, unsigned value)
    {
        for (int b = 0; b < kBits; ++b) {
            charge_[row][b] = ((value >> b) & 1u) ? 1.0 : 0.0;
        }
    }

    // Reading senses every cell of the row and writes the row back at full charge.
    unsigned read(int row)
    {
        unsigned value = 0;
        for (int b = 0; b < kBits; ++b) {
            if (charge_[row][b] > kThreshold) {
                value |= 1u << b;
            }
        }
        write(row, value);
        return value;
    }

    void refresh(int row) { read(row); }    // a refresh is a read whose result is unused

    void leak()
    {
        for (auto& row : charge_) {
            for (double& c : row) {
                c *= kKeepPerTick;
            }
        }
    }

    double peek(int row, int bit) const { return charge_[row][bit]; }   // model only

private:
    std::array<std::array<double, kBits>, kRows> charge_{};
};

unsigned pattern(int row)
{
    return 0xA5u ^ static_cast<unsigned>(row);   // a different byte in every row
}

int check(Dram& dram)
{
    int lost = 0;
    for (int r = 0; r < kRows; ++r) {
        const unsigned got = dram.read(r);
        std::printf("  row %d: wrote 0x%02X read 0x%02X %s\n", r, pattern(r), got,
                    got == pattern(r) ? "OK" : "LOST");
        lost += (got != pattern(r));
    }
    return lost;
}

int main()
{
    // A: no refresh at all. Watch the charge of one stored 1 fade.
    Dram a;
    for (int r = 0; r < kRows; ++r) {
        a.write(r, pattern(r));
    }
    std::printf("A: no refresh. Charge of row 0, bit 0 (a stored 1):\n");
    for (int tick = 1; tick <= 16; ++tick) {
        a.leak();
        std::printf("  tick %2d: %.3f%s\n", tick, a.peek(0, 0),
                    a.peek(0, 0) > kThreshold ? "" : "  (below the threshold: reads as 0)");
    }
    std::printf("A: reading all rows after 16 ticks:\n");
    const int lostA = check(a);

    // B: refresh one row per tick, in turn: every row is refreshed every kRows ticks.
    Dram b;
    for (int r = 0; r < kRows; ++r) {
        b.write(r, pattern(r));
    }
    int row = 0;                                 // the refresh counter
    for (int tick = 1; tick <= 40; ++tick) {
        b.leak();
        b.refresh(row);
        row = (row + 1) % kRows;
    }
    std::printf("B: refresh one row per tick, 40 ticks. Reading all rows:\n");
    const int lostB = check(b);

    std::printf("rows lost: A = %d, B = %d\n", lostA, lostB);
    return 0;
}
