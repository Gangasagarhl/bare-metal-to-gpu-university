// F0-61 Listing 2: a robot updates its belief that a door is open, one noisy reading
// at a time (a discrete Bayes filter with two states and no motion).
#include <iostream>
#include <vector>

struct Sensor
{
    double pSaysOpenIfOpen;    // P(reading "open" | door open)
    double pSaysOpenIfClosed;  // P(reading "open" | door closed)
};

// One Bayes update: multiply the prior by the likelihood of the reading, then normalise.
double update(double beliefOpen, bool saysOpen, const Sensor& s)
{
    const double likeOpen = saysOpen ? s.pSaysOpenIfOpen : 1.0 - s.pSaysOpenIfOpen;
    const double likeClosed = saysOpen ? s.pSaysOpenIfClosed : 1.0 - s.pSaysOpenIfClosed;
    const double unnormOpen = likeOpen * beliefOpen;
    const double unnormClosed = likeClosed * (1.0 - beliefOpen);
    return unnormOpen / (unnormOpen + unnormClosed);
}

int main()
{
    const Sensor sensor{0.6, 0.2};   // exercise values for a cheap distance sensor
    double belief = 0.5;             // no idea at the start
    const std::vector<bool> readings{true, true, false, true, true, true};
    std::cout << "step  reading  P(open)\n";
    std::cout << "0     -        " << belief << "\n";
    int step = 1;
    for (bool r : readings) {
        belief = update(belief, r, sensor);
        std::cout << step << "     " << (r ? "open  " : "closed") << "   " << belief << "\n";
        ++step;
    }
    return 0;
}
