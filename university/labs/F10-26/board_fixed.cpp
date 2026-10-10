// board_fixed.cpp - checks the board description given on standard input (board_fixed.in).
#include <iostream>

#include "board_check.h"

int main()
{
    return checkBoard(std::cin).errors == 0 ? 0 : 1;
}
