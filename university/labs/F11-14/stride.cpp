// F11-14 Listing 1 (copied unchanged into F11-15 to F11-17): read a data-flow diagram from
// standard input and print its boundary crossings and STRIDE threats. Exit code 1 when
// the diagram breaks a drawing rule.
#include <iostream>

#include "tm.h"

int main()
{
    const tmod::Model m = tmod::parse(std::cin);
    return tmod::report(m);
}
