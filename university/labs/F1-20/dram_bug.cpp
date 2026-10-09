// Forensic evidence: Lena's refresh controller, in the same DRAM teaching model.
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
    Dram dram;
    for (int r = 0; r < kRows; ++r) {
        dram.write(r, pattern(r));
    }
    unsigned row = 0;                            // the refresh counter: 2 bits
    std::printf("refresh log (first 12 ticks):\n");
    for (int tick = 1; tick <= 40; ++tick) {
        dram.leak();
        dram.refresh(static_cast<int>(row));
        if (tick <= 12) {
            std::printf("  tick %2d: refreshed row %u\n", tick, row);
        }
        row = (row + 1) & 0x3u;                  // keep the counter in its 2 bits
    }
    std::printf("after 40 ticks, reading all rows:\n");
    const int lost = check(dram);
    std::printf("rows lost: %d\n", lost);
    return 0;
}
