// bad.cc - F11-11: a parser-style function written the careless way, so a static
// analyser (clang-tidy with CERT and bugprone checks) has something to find. Each
// problem here is a well-known coding-standard violation:
//   - atoi: no error reporting on bad input (CERT ERR34-C / cert-err34-c);
//   - assignment inside an if condition instead of a comparison (bugprone);
//   - use of an uninitialised variable (CERT, analyzer);
//   - freeing the same pointer twice (double free).
// run.sh runs clang-tidy over this file and saves the report. The file still
// compiles, which is the point: the compiler is happy, the analyser is not.
#include <cstdio>
#include <cstring>
#include <cstdlib>

int parse_count(const char* text)
{
    int n = std::atoi(text);                         // ERR34-C: silent on bad input
    char* p = static_cast<char*>(std::malloc(static_cast<size_t>(n)));
    if (p = nullptr) std::printf("allocation failed\n");  // '=' not '==' (bugprone)
    int chosen;                                      // never initialised
    std::printf("count=%d chosen=%d\n", n, chosen);  // uses uninitialised 'chosen'
    std::free(p);
    std::free(p);                                    // double free
    return n;
}

int main(int argc, char** argv)
{
    return parse_count(argc > 1 ? argv[1] : "0");
}
