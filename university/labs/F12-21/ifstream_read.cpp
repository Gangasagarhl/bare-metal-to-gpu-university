// ifstream_read.cpp - four reads in C++; how many read() system calls reach the kernel?
// run.sh runs this program under strace to show the answer from the kernel's side.
#include <fstream>
#include <iostream>
#include <vector>

int main()
{
    std::ifstream in("sample.txt", std::ios::binary);  // 22,000 bytes of text
    std::vector<char> buf(20000);
    for (std::streamsize want : {16, 16, 12000, 20000}) {
        in.read(buf.data(), want);
        std::cout << "C++ in.read(buf, " << want << ") returned " << in.gcount() << " bytes\n";
    }
    return 0;
}
