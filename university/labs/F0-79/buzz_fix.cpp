// F0-79 forensic answer-key run: the 10 ms loop with dt taken from the real loop period.
#include "buzz.hpp"

int main()
{
    run("fixed build", 0.010, 0.010);
    return 0;
}
