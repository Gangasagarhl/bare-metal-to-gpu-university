// F1-69 Listing 3: six-step commutation of a brushless (BLDC) motor from three
// Hall sensors. The table belongs to the university's PRETEND motor; a real
// motor's Hall-to-phase table comes from its datasheet or its driver's datasheet.
// For each Hall pattern, one phase is driven high (+), one low (-), one floats (.).
#include <array>
#include <cstdio>
#include <string>

// Hall pattern (H1 H2 H3 as a 3-bit number) -> phases A, B, C.
std::string commutate(int hall)
{
    switch (hall) {
    case 0b101: return "+-.";  // A high, B low
    case 0b100: return "+.-";  // A high, C low
    case 0b110: return ".+-";  // B high, C low
    case 0b010: return "-+.";  // B high, A low
    case 0b011: return "-.+";  // C high, A low
    case 0b001: return ".-+";  // C high, B low
    default: return "...";     // 000 and 111 never occur in a healthy motor: all off
    }
}

// Hall sensors of the pretend motor, 120 electrical degrees apart.
int hallFromAngle(int deg, bool h2Broken)
{
    const int h1 = (deg >= 0 && deg < 180) ? 1 : 0;
    const int h2 = h2Broken ? 0 : ((deg >= 120 && deg < 300) ? 1 : 0);
    const int h3 = (deg >= 240 || deg < 60) ? 1 : 0;
    return (h1 << 2) | (h2 << 1) | h3;
}

void run(const char* title, bool h2Broken)
{
    std::printf("%s\n  angle  H1H2H3  A B C\n", title);
    for (int deg = 30; deg < 360; deg += 60) {
        const int h = hallFromAngle(deg, h2Broken);
        const std::string p = commutate(h);
        std::printf("  %4d     %d%d%d    %c %c %c%s\n", deg, (h >> 2) & 1, (h >> 1) & 1, h & 1,
                    p[0], p[1], p[2], p == "..." ? "   <- invalid pattern, outputs off" : "");
    }
    std::printf("\n");
}

int main()
{
    run("healthy Hall sensors (one line per 60 electrical degrees):", false);
    run("Hall sensor H2 wire broken (always reads 0):", true);
    return 0;
}
