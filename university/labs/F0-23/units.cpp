// Bits, bytes and the two kinds of "kilo": 1000 (kB) and 1024 (KiB).
#include <climits>
#include <iostream>

int main()
{
    std::cout << "bits in one byte on this machine: " << CHAR_BIT << '\n';
    std::cout << "bytes in an int on this machine: " << sizeof(int) << '\n';
    std::cout << "bytes in a pointer on this machine: " << sizeof(void*) << "\n\n";

    const long long kB = 1000;
    const long long KiB = 1024;
    std::cout << "1 kB  = " << kB << " bytes\n";
    std::cout << "1 KiB = " << KiB << " bytes\n";
    std::cout << "1 MB  = " << kB * kB << " bytes\n";
    std::cout << "1 MiB = " << KiB * KiB << " bytes\n";
    std::cout << "1 GB  = " << kB * kB * kB << " bytes\n";
    std::cout << "1 GiB = " << KiB * KiB * KiB << " bytes\n\n";

    const long long fileSize = 5000000;  // a file of five million bytes
    std::cout << "a file of " << fileSize << " bytes is\n";
    std::cout << "  " << static_cast<double>(fileSize) / (kB * kB) << " MB\n";
    std::cout << "  " << static_cast<double>(fileSize) / (KiB * KiB) << " MiB\n";
    std::cout << "  " << fileSize * 8 << " bits\n";
    return 0;
}
