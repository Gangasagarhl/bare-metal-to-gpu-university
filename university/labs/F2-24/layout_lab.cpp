#include <cstddef>
#include <cstdint>
#include <iostream>

// Lab: write your prediction in the comment, then run and compare.
struct A { char c; int i; };                         // prediction: sizeof = ?
struct B { int i; char c; };                         // prediction: sizeof = ?
struct C { char c1; char c2; int i; };               // prediction: sizeof = ?
struct D { double d; char c; };                      // prediction: sizeof = ?
struct E { char c; double d; char c2; };             // prediction: sizeof = ?
struct F { std::uint16_t a; std::uint8_t b; };       // prediction: sizeof = ?
struct G { };                                        // prediction: sizeof = ?

int main()
{
    std::cout << "A " << sizeof(A) << " (align " << alignof(A) << ")\n";
    std::cout << "B " << sizeof(B) << " (align " << alignof(B) << ")\n";
    std::cout << "C " << sizeof(C) << " (align " << alignof(C) << ")\n";
    std::cout << "D " << sizeof(D) << " (align " << alignof(D) << ")\n";
    std::cout << "E " << sizeof(E) << " (align " << alignof(E) << ")\n";
    std::cout << "F " << sizeof(F) << " (align " << alignof(F) << ")\n";
    std::cout << "G " << sizeof(G) << " (align " << alignof(G) << ")\n";
    return 0;
}
