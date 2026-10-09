/* ctest.c - F3-35: a C test program that must print the same on every C library it is built with
 * (curriculum B18, second acceptance test). It uses only what tlibc provides. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checksum(const char* s)          /* a tiny rolling hash of a string */
{
    unsigned h = 0;
    while (*s) h = h * 31u + (unsigned char)*s++;
    return h;
}

int main(int argc, char** argv)
{
    (void)argv;
    const char* words[] = {"files", "pipes", "shells", "libraries"};
    printf("ctest: argc = %d\n", argc);
    for (int i = 0; i < 4; ++i)
        printf("%-10s length %2u checksum %08x\n", words[i], (unsigned)strlen(words[i]), checksum(words[i]));
    long a = 0, b = 1;
    printf("fibonacci:");
    for (int i = 0; i < 12; ++i) { printf(" %ld", a); long t = a + b; a = b; b = t; }
    putchar('\n');
    printf("negative %d, padded [%05d] [%5d], hex %x, char %c, percent %%\n", -42, 42, -7, 48879, 'Z');
    printf("abs(-9) = %d, strcmp(\"abc\", \"abd\") < 0: %s\n", abs(-9), strcmp("abc", "abd") < 0 ? "yes" : "no");
    puts("ctest: done");
    return 0;
}
