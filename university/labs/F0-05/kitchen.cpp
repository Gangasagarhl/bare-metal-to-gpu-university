// A toy model of the kitchen map in chapter F0-05. It is NOT a real CPU:
// real processors have their own exact instruction lists (HW102 and HW202).
#include <iostream>
#include <string>
#include <vector>

struct Step
{
    std::string action;
    int box;
};

int main()
{
    std::vector<int> pantry = {7, 5, 0};
    int leftHand = 0;
    int rightHand = 0;

    const std::vector<Step> recipe = {
        {"LOAD_LEFT", 0}, {"LOAD_RIGHT", 1}, {"ADD", 0}, {"STORE_LEFT", 2},
    };

    for (const Step& step : recipe) {
        if (step.action == "LOAD_LEFT") {
            leftHand = pantry[step.box];
        } else if (step.action == "LOAD_RIGHT") {
            rightHand = pantry[step.box];
        } else if (step.action == "ADD") {
            leftHand = leftHand + rightHand;
        } else if (step.action == "STORE_LEFT") {
            pantry[step.box] = leftHand;
        }
        std::cout << step.action << " " << step.box << "  -> hands: " << leftHand << ", "
                  << rightHand << "  pantry boxes: " << pantry[0] << ", " << pantry[1]
                  << ", " << pantry[2] << '\n';
    }
    return 0;
}
