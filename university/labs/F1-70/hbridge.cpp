// F1-70 Listing 1: the H-bridge truth model.
// Four switches: AH and AL on the motor's terminal A (high side to the supply,
// low side to ground), BH and BL on terminal B. This program lists all 16 switch
// combinations and says what each does. Ideal switches; no real part.
#include <cstdio>
#include <string>

std::string classify(bool ah, bool al, bool bh, bool bl)
{
    if ((ah && al) || (bh && bl)) {
        return "SHOOT-THROUGH: supply shorted to ground (forbidden)";
    }
    if (ah && bl && !bh && !al) {
        return "forward: current A -> B through the motor";
    }
    if (bh && al && !ah && !bl) {
        return "reverse: current B -> A through the motor";
    }
    if (al && bl) {
        return "brake (low side): motor terminals shorted together";
    }
    if (ah && bh) {
        return "brake (high side): motor terminals shorted together";
    }
    return "coast: no driven path (current can only decay)";
}

int main()
{
    std::printf("AH AL BH BL  meaning\n");
    int forbidden = 0;
    for (int m = 0; m < 16; ++m) {
        const bool ah = m & 8;
        const bool al = m & 4;
        const bool bh = m & 2;
        const bool bl = m & 1;
        const std::string s = classify(ah, al, bh, bl);
        if (s.rfind("SHOOT", 0) == 0) {
            ++forbidden;
        }
        std::printf(" %d  %d  %d  %d  %s\n", ah, al, bh, bl, s.c_str());
    }
    std::printf("%d of 16 combinations are forbidden\n", forbidden);
    return 0;
}
