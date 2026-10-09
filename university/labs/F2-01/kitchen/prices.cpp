// The kitchen's price list, in its own source file.
int dish_price(int dish_number)
{
    if (dish_number == 1) {
        return 7;   // soup
    }
    if (dish_number == 2) {
        return 12;  // noodles
    }
    return 0;       // a dish the kitchen does not know
}
