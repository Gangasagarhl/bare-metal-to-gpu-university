// forensic_bringup.cpp - boots the model board with the description on standard input (forensic_bringup.in).
#include <iostream>

#include "bringup.h"

int main()
{
    return boot(std::cin);
}
