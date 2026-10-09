// throw.cpp: exceptions in code compiled with -fno-exceptions.
int checkedDivide(int a, int b)
{
    if (b == 0) {
        throw "division by zero";
    }
    return a / b;
}
