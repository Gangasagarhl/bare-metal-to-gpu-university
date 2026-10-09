// Evidence program for the forensic lab: moving marbles between two jars.
// Rule of the game: marbles move between jars; none are added or taken away.
#include <iostream>
#include <vector>

int main()
{
    int red = 12;   // marbles in the red jar
    int blue = 8;   // marbles in the blue jar
    const int total = red + blue;
    // positive: move that many from red to blue; negative: from blue to red
    const std::vector<int> moves = {3, -2, 4, -1};
    std::cout << "start   red=" << red << " blue=" << blue << " sum=" << red + blue << '\n';
    int step = 0;
    for (int move : moves) {
        ++step;
        if (move > 0) {
            red = red - move;
            blue = blue + move;
        } else {
            blue = blue + move;
            red = red + move;
        }
        std::cout << "move " << step << "  red=" << red << " blue=" << blue
                  << " sum=" << red + blue
                  << (red + blue == total ? "" : "   <-- sum changed") << '\n';
    }
    return 0;
}
