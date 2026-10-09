// hello.cpp: the sample program whose ELF files the lab reads.
#include <cstdio>

int greetings = 0;                 // .bss after linking (zero-initialised)
const char banner[] = "ELF lab";   // .rodata

int greet(const char* who)
{
    ++greetings;
    return std::printf("%s: hello, %s\n", banner, who);
}

int main()
{
    greet("reader");
    return greetings == 1 ? 0 : 1;
}
