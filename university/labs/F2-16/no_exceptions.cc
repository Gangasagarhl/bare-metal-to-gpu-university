#include <stdexcept>

int portions_or_throw(int n)
{
    if (n < 0) {
        throw std::invalid_argument("negative portions");
    }
    return n;
}

int main()
{
    return portions_or_throw(3);
}
