#include <iostream>
#include <vector>

// Forensic evidence: the evening report adds up tips and reads one element too many.
int main()
{
    std::cout << std::unitbuf;
    const std::vector<int> tips = {4, 9, 2, 7};
    int sum = 0;
    for (std::size_t i = 0; i <= tips.size(); ++i) {    // <= instead of <
        sum += tips[i];
    }
    std::cout << "tips today: " << sum << '\n';
    return 0;
}
