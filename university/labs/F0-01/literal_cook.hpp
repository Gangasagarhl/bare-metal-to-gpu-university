// The "literal cook": a tiny simulator written for KID101 (chapter F0-01).
// It reads one instruction per line and does exactly what the line says,
// nothing more. Unknown lines are skipped. At the end it describes the plate.
// It is our own teaching model, not a model of any real machine.
#include <iostream>
#include <string>

struct Kitchen
{
    bool bagOpen = false;
    bool butterOpen = false;
    bool knifeHasButter = false;
    int slicesOnPlate = 0;
    bool firstSliceButtered = false;
    bool cheeseOnFirstSlice = false;
    bool secondSliceOnTop = false;
};

inline void doStep(Kitchen& k, const std::string& step)
{
    if (step == "OPEN THE BREAD BAG") {
        k.bagOpen = true;
    } else if (step == "PUT A SLICE OF BREAD ON THE PLATE") {
        if (k.bagOpen) {
            k.slicesOnPlate = k.slicesOnPlate + 1;
        }
    } else if (step == "OPEN THE BUTTER TUB") {
        k.butterOpen = true;
    } else if (step == "DIP THE KNIFE IN THE BUTTER") {
        if (k.butterOpen) {
            k.knifeHasButter = true;
        }
    } else if (step == "SPREAD THE KNIFE ON THE FIRST SLICE") {
        if (k.knifeHasButter && k.slicesOnPlate >= 1) {
            k.firstSliceButtered = true;
            k.knifeHasButter = false;
        }
    } else if (step == "PUT CHEESE ON THE FIRST SLICE") {
        if (k.slicesOnPlate >= 1) {
            k.cheeseOnFirstSlice = true;
        }
    } else if (step == "PUT THE SECOND SLICE ON TOP") {
        if (k.slicesOnPlate >= 2) {
            k.secondSliceOnTop = true;
        }
    }
}

inline int runCook(std::istream& in)
{
    Kitchen k;
    std::string line;
    int number = 0;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        number = number + 1;
        std::cout << "Step " << number << ": " << line << '\n';
        doStep(k, line);
    }
    std::cout << "\nWhat is on the plate at the end:\n";
    std::cout << "  slices of bread on the plate: " << k.slicesOnPlate << '\n';
    std::cout << "  first slice has butter:       " << (k.firstSliceButtered ? "yes" : "no") << '\n';
    std::cout << "  cheese on the first slice:    " << (k.cheeseOnFirstSlice ? "yes" : "no") << '\n';
    std::cout << "  second slice on top:          " << (k.secondSliceOnTop ? "yes" : "no") << '\n';
    std::cout << "  butter tub left open:         " << (k.butterOpen ? "yes" : "no") << '\n';
    return 0;
}
