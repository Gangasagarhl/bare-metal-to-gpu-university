// HW204 practical (P), Lab Engineer's evidence for the key (NOT handed to candidates):
// the hand-decoding step asks for the sample number of every START and STOP and the nine
// bits of every byte. This program reads the exam capture (i2c_decode.in, identical to the
// candidates' i2c_decode_start.in) and prints, for every event, the sample index at which it
// is seen (the index of the second of the two samples compared, counting from 0 as the row
// labels of the capture do), and for every byte the samples of its nine rising SCL edges.
// Same rules as the reference decoder i2c_decode.cpp (F1-47, Layer 2).
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

struct Sample { int scl; int sda; };

int main()
{
    std::ifstream in("i2c_decode.in");
    if (!in) { std::printf("cannot open i2c_decode.in\n"); return 2; }
    std::vector<Sample> tr;
    std::string line, scl_row;
    while (std::getline(in, line)) {
        const auto p = line.find("SCL ");
        const auto q = line.find("SDA ");
        if (p != std::string::npos) scl_row = line.substr(p + 4);
        else if (q != std::string::npos) {
            const std::string sda_row = line.substr(q + 4);
            for (std::size_t i = 0; i < scl_row.size() && i < sda_row.size(); ++i)
                tr.push_back({scl_row[i] == '-' ? 1 : 0, sda_row[i] == '-' ? 1 : 0});
            scl_row.clear();
        }
    }
    std::printf("capture: %zu samples (index 0 to %zu)\n", tr.size(), tr.size() - 1);
    std::vector<std::size_t> edges;
    std::vector<int> bits;
    bool first = false;
    bool open = false;
    int transaction = 1;
    auto flush = [&]() {
        if (bits.size() == 9) {
            int v = 0;
            for (int i = 0; i < 8; ++i) v = (v << 1) | bits[i];
            std::printf("    %s bits ", first ? "address" : "data   ");
            for (int i = 0; i < 8; ++i) std::printf("%d%s", bits[i], i == 3 ? " " : "");
            std::printf(" | %d  = 0x%02X %s", bits[8], v, bits[8] == 0 ? "ACK " : "NACK");
            if (first) std::printf("  (7-bit 0x%02X, %s)", v >> 1, (v & 1) ? "R" : "W");
            std::printf("  rising SCL at samples");
            for (std::size_t e : edges) std::printf(" %zu", e);
            std::printf("\n");
            first = false;
        }
        bits.clear();
        edges.clear();
    };
    for (std::size_t i = 1; i < tr.size(); ++i) {
        const Sample a = tr[i - 1], b = tr[i];
        if (a.scl && b.scl && a.sda && !b.sda) {
            flush();
            std::printf("  START at sample %zu%s\n", i, open ? "  (repeated START: no STOP before it)" : "");
            first = true;
            open = true;
        } else if (a.scl && b.scl && !a.sda && b.sda) {
            flush();
            std::printf("  STOP  at sample %zu  (end of transaction %d)\n", i, transaction++);
            open = false;
        } else if (!a.scl && b.scl) {
            bits.push_back(b.sda);
            edges.push_back(i);
            if (bits.size() == 9) flush();
        }
    }
    return 0;
}
