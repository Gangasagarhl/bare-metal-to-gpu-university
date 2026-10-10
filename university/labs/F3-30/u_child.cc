// u_child.cc - exits with status 10 + its argument, so every child's status differs.
#include "ulib.h"

int main(int argc, char** argv)
{
    int n = argc > 1 ? argv[1][0] - '0' : 0;
    return 10 + n;
}
