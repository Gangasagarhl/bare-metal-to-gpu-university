// hello.cc - a trivial program built two ways by run.sh so that readelf can show
// the difference between a weakly built binary and a hardened one.
#include <cstdio>
int main() { std::puts("hello"); return 0; }
