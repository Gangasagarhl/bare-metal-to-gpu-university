template <typename T>
T larger(T a, T b)
{
    return (a < b) ? b : a;
}

int main()
{
    return static_cast<int>(larger(3, 2.5));   // T = int or T = double? The compiler cannot choose.
}
