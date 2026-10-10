// sort_list.cpp: tries to sort a std::list with std::sort. It does not compile.
#include <algorithm>
#include <cstdio>
#include <list>

int main()
{
    std::list<int> waiting = {42, 7, 19};
    std::sort(waiting.begin(), waiting.end());
    for (int ticket : waiting) {
        std::printf("%d\n", ticket);
    }
    return 0;
}
