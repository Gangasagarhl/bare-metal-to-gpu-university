// guard_unit.cpp - MP7 milestone 1, step 1: the unit tests of our estimator guard.
#include "guard_tests.h"

int main()
{
    return mp7::run_suite<mp7::EstGuard>("unit tests: est_guard.h (EstGuard)") == 0 ? 0 : 1;
}
