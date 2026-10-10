// F1-47 Listing 2: UART framing. The line idles high; each byte is sent as a
// start bit (low), 8 data bits least-significant bit first, and a stop bit
// (high): "8N1". The receiver finds the falling edge of the start bit and then
// samples in the middle of each bit time. If its bit time is wrong, it reads
// garbage. 8 samples per bit time in this model.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

std::vector<int> encode(const std::string& s, int spb)
{
    std::vector<int> line(spb * 2, 1);                 // idle
    for (unsigned char ch : s) {
        std::vector<int> bits{0};                      // start bit
        for (int i = 0; i < 8; ++i) bits.push_back((ch >> i) & 1);   // LSB first
        bits.push_back(1);                             // stop bit
        for (int b : bits) line.insert(line.end(), static_cast<std::size_t>(spb), b);
    }
    line.insert(line.end(), static_cast<std::size_t>(spb * 2), 1);
    return line;
}

std::string decode(const std::vector<int>& line, double spb)
{
    std::string out;
    std::size_t i = 1;
    while (i < line.size()) {
        if (line[i - 1] == 1 && line[i] == 0) {          // start bit edge
            int v = 0;
            for (int bit = 0; bit < 8; ++bit) {
                const auto at = static_cast<std::size_t>(i + spb * (1.5 + bit));
                if (at >= line.size()) return out;
                v |= line[at] << bit;
            }
            const auto stop_at = static_cast<std::size_t>(i + spb * 9.5);
            const bool framing_ok = stop_at < line.size() && line[stop_at] == 1;
            char buf[32];
            std::snprintf(buf, sizeof buf, "[0x%02X%s]", v, framing_ok ? "" : " FRAMING ERROR");
            out += buf;
            i = static_cast<std::size_t>(i + spb * 9.5);   // continue after the stop bit
        }
        ++i;
    }
    return out;
}

int main()
{
    const int spb = 8;
    const std::vector<int> line = encode("Hi", spb);
    std::printf("'H' = 0x48 = 0100 1000; sent LSB first: 0 0 0 1 0 0 1 0\n");
    std::string wave;
    for (std::size_t i = 0; i < line.size(); i += 2) wave += line[i] ? '-' : '_';
    std::printf("line (every 2nd sample): %s\n", wave.c_str());
    std::printf("receiver, same bit time      : %s\n", decode(line, spb).c_str());
    std::printf("receiver, bit time 25 %% long : %s\n", decode(line, spb * 1.25).c_str());
    std::printf("receiver, bit time 25 %% short: %s\n", decode(line, spb * 0.75).c_str());
    return 0;
}
