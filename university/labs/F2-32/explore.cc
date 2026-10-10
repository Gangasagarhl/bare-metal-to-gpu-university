// explore.cc: four tiny functions to read in the compiler's assembly output.
unsigned divide_by_8(unsigned x)
{
    return x / 8;
}

int divide_by_8_signed(int x)
{
    return x / 8;
}

bool in_range(int x)
{
    return x >= 10 && x < 20;
}

int square(int x)
{
    return x * x;
}
