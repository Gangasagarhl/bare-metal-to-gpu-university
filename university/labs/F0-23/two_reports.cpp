// Evidence program for the forensic lab: two size reports for the same download.
#include <iostream>

void reportA(long long bytes)
{
    std::cout << "Report A: " << static_cast<double>(bytes) / 1000000 << " MB\n";
}

void reportB(long long bytes)
{
    std::cout << "Report B: " << static_cast<double>(bytes) / (1024 * 1024) << " MB\n";
}

int main()
{
    const long long download = 3145728;
    std::cout << "download size: " << download << " bytes\n";
    reportA(download);
    reportB(download);
    return 0;
}
