#include <iostream>

// Lab: walk an array with a pointer and return a pointer to the largest element.
int* largest(int* first, int* last)   // [first, last): last is one past the end
{
    int* best = first;
    for (int* p = first; p != last; ++p) {
        if (*p > *best) {
            best = p;
        }
    }
    return best;
}

int main()
{
    int tips[6] = {4, 9, 2, 9, 7, 1};
    int* const begin = &tips[0];
    int* const end = begin + 6;       // one past the end: may be computed, never followed

    int* top = largest(begin, end);
    std::cout << "largest tip " << *top << " at index " << (top - begin) << '\n';
    *top = 0;                         // change the array through the pointer
    top = largest(begin, end);
    std::cout << "after zeroing it, largest tip " << *top << " at index " << (top - begin) << '\n';
    return 0;
}
