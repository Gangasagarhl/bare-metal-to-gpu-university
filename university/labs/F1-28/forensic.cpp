// F1-28 forensic evidence: the branch "if (value >= 128)" inside a loop over 64 values,
// predicted by a 2-bit counter, once for yesterday's data and once for today's data.
#include <cstdint>
#include <cstdio>
#include <vector>

static int run(const char* label, const std::vector<int>& values)
{
    int counter = 0, wrong = 0;   // 2-bit counter: 0,1 predict not taken; 2,3 predict taken
    std::printf("%-10s outcomes: ", label);
    for (std::size_t i = 0; i < values.size(); ++i) {
        const bool taken = values[i] >= 128;
        const bool guess = counter >= 2;
        wrong += guess != taken;
        counter = taken ? (counter < 3 ? counter + 1 : 3) : (counter > 0 ? counter - 1 : 0);
        if (i < 40) {
            std::printf("%c", taken ? 'T' : 'N');
        }
    }
    std::printf("...  mispredictions %d of %zu\n", wrong, values.size());
    return wrong;
}

int main()
{
    std::vector<int> today;
    std::uint32_t x = 12345;                 // fixed seed: the same "random" data every run
    for (int i = 0; i < 64; ++i) {
        x = x * 1103515245u + 12345u;        // a linear congruential generator
        today.push_back(static_cast<int>((x >> 16) % 256));
    }
    std::vector<int> yesterday = today;      // same values ...
    for (std::size_t i = 1; i < yesterday.size(); ++i) {   // ... sorted (insertion sort)
        for (std::size_t j = i; j > 0 && yesterday[j - 1] > yesterday[j]; --j) {
            std::swap(yesterday[j - 1], yesterday[j]);
        }
    }
    int below = 0;
    for (const int v : today) {
        below += v < 128;
    }
    std::printf("64 values, %d of them below 128 (the same values on both days)\n", below);
    const int w1 = run("yesterday", yesterday);
    const int w2 = run("today", today);
    std::printf("cycles lost at 2 per misprediction: yesterday %d, today %d\n", 2 * w1, 2 * w2);
    return 0;
}
