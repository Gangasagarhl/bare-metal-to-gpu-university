// wom_path.cc: "works on my machine" trap 2. A test that reads its input file through
// a path relative to the current directory. It passes when you start it from the
// project folder and fails when the test runner starts it from somewhere else.
#include <cstdio>
#include <fstream>
#include <string>

int main()
{
    std::ifstream in("tests/data/sample.txt");
    if (!in) {
        std::printf("test FAIL: cannot open tests/data/sample.txt\n");
        return 1;
    }
    std::string first_line;
    std::getline(in, first_line);
    std::printf("test PASS: first line \"%s\"\n", first_line.c_str());
    return 0;
}
