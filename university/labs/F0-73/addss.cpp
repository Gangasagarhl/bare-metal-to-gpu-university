// F0-73 hardware look: what instruction does the compiler use for a float and a double add?
float addFloat(float a, float b)
{
    return a + b;
}

double addDouble(double a, double b)
{
    return a + b;
}

int main()
{
    return static_cast<int>(addFloat(1.0f, 2.0f) + static_cast<float>(addDouble(3.0, 4.0))) == 10
               ? 0
               : 1;
}
