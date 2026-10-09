// probe.cc: what does parse_duration do with a number too large for a long?
#include "duration.h"

#include <cstdio>

int main()
{
    const auto d = parse_duration("99999999999999999999s");
    std::printf("has value: %d\n", d.has_value() ? 1 : 0);
    return 0;
}
