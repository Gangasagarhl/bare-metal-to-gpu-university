// reserve.cc: reads past size() but inside the vector's reserved capacity.
#include <cstdio>
#include <vector>

int main()
{
    std::vector<int> v;
    v.reserve(8);
    for (int i = 0; i < 5; ++i) {
        v.push_back(i * 10);
    }
    std::printf("v[6] = %d\n", v[6]);         // bug: index 6, but size() is 5
    return 0;
}
