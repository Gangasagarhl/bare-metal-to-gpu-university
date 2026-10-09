// F1-30 Listing 2: why "count = count + 1" on two cores can lose an update. Each core does
// three steps: LOAD the shared value into its register, ADD 1, STORE it back. This program
// plays the steps in a chosen order (an interleaving) and prints what memory holds.
#include <cstdio>
#include <string>

static void play(const std::string& order)
{
    int memory = 0;
    int reg[2] = {0, 0};
    int step[2] = {0, 0};             // next step of each core: 0 LOAD, 1 ADD, 2 STORE
    const char* names[3] = {"LOAD ", "ADD 1", "STORE"};
    std::printf("order %s\n", order.c_str());
    for (const char ch : order) {
        const int c = ch - 'A';       // 'A' = core 0, 'B' = core 1
        switch (step[c]) {
        case 0: reg[c] = memory; break;
        case 1: reg[c] = reg[c] + 1; break;
        default: memory = reg[c]; break;
        }
        std::printf("  core %c %s  reg A=%d reg B=%d  memory=%d\n", ch, names[step[c]], reg[0],
                    reg[1], memory);
        ++step[c];
    }
    std::printf("  final memory = %d (two increments were done)\n", memory);
}

int main()
{
    play("AAABBB");   // core A finishes before core B starts
    play("ABABAB");   // the steps alternate: both load the old value 0
    return 0;
}
