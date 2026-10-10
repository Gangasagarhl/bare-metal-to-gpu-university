#include "checksum.h"

#include <cstdio>

int main()
{
    std::printf("checksum(\"packer\") = %u\n", checksum("packer"));
    return 0;
}
