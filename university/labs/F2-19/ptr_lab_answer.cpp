#include <iostream>

// Lab reference solution: Listing 4 plus findFirst and addToAll (steps 3 and 4).
int* largest(int* first, int* last)
{
    int* best = first;
    for (int* p = first; p != last; ++p) {
        if (*p > *best) {
            best = p;
        }
    }
    return best;
}

int* findFirst(int* first, int* last, int value)   // returns last if not found
{
    for (int* p = first; p != last; ++p) {
        if (*p == value) {
            return p;
        }
    }
    return last;
}

void addToAll(int* first, int* last, int amount)
{
    for (int* p = first; p != last; ++p) {
        *p += amount;
    }
}

int main()
{
    int tips[6] = {4, 9, 2, 9, 7, 1};
    int* const begin = &tips[0];
    int* const end = begin + 6;

    int* top = largest(begin, end);
    *top = 0;
    std::cout << "findFirst(7) at index " << (findFirst(begin, end, 7) - begin) << '\n';
    std::cout << "findFirst(100) at index " << (findFirst(begin, end, 100) - begin)
              << (findFirst(begin, end, 100) == end ? " (== end: not found)" : "") << '\n';
    addToAll(begin + 3, end, 1);
    std::cout << "array now:";
    for (const int* p = begin; p != end; ++p) {
        std::cout << ' ' << *p;
    }
    std::cout << '\n';
    return 0;
}
