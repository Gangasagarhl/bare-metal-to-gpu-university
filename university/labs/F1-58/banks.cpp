// F1-58 Listing 3: a bank-conflict model for one group of threads reading a 2-D tile.
// Model parameters (check them in YOUR vendor guide; see the chapter's unverified box):
//   banks      number of banks; bankWidth bytes per bank word.
// Address of tile[row][col] with 4-byte elements = (row * pitch + col) * 4.
// Bank = (address / bankWidth) % banks. Conflict degree = most threads on one bank
// (threads reading the very same word are counted once: one read serves them).
#include <cstdio>
#include <map>
#include <set>

int degree(int threads, int pitch, bool columnRead, int banks, int bankWidth)
{
    std::map<int, std::set<long>> wordsPerBank;
    for (int t = 0; t < threads; ++t) {
        int row = columnRead ? t : 0;     // column read: thread t reads tile[t][0]
        int col = columnRead ? 0 : t;     // row read:    thread t reads tile[0][t]
        long address = (static_cast<long>(row) * pitch + col) * 4;
        long word = address / bankWidth;
        wordsPerBank[static_cast<int>(word % banks)].insert(word);
    }
    int worst = 0;
    for (const auto& kv : wordsPerBank) {
        int n = static_cast<int>(kv.second.size());
        worst = n > worst ? n : worst;
    }
    return worst;
}

int main()
{
    const int banks = 32, bankWidth = 4, threads = 32;   // model parameters, see above
    std::printf("model: %d banks of %d bytes, %d threads reading at once\n", banks, bankWidth, threads);
    std::printf("%-8s %-12s %s\n", "pitch", "access", "conflict degree (1 = no conflict)");
    for (int pitch : {32, 33, 64, 65}) {
        std::printf("%-8d %-12s %d\n", pitch, "row", degree(threads, pitch, false, banks, bankWidth));
        std::printf("%-8d %-12s %d\n", pitch, "column", degree(threads, pitch, true, banks, bankWidth));
    }
    return 0;
}
