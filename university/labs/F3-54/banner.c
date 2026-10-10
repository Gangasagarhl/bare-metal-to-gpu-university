/* banner.c - F3-54 forensic: the start-up banner of a ported program, as its upstream wrote it. */
#include <stdio.h>

int main(void)
{
    puts("banner-port 1.0 for myos");
    puts("build: " __DATE__ " " __TIME__);
    return 0;
}
