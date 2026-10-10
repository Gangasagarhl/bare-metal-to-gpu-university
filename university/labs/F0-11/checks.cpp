// Number check for F0-11: recomputes every number used in the chapter text.
#include <iostream>

int main()
{
    std::cout << "3407 in parts: " << 3 * 1000 << " + " << 4 * 100 << " + " << 0 * 10
              << " + " << 7 << " = " << 3 * 1000 + 4 * 100 + 0 * 10 + 7 << "\n";
    std::cout << "whole tens in 3407: " << 3407 / 10 << ", ones left: " << 3407 % 10 << "\n";
    std::cout << "347 beans: bowls of 100 = " << 347 / 100 << ", cups of 10 = " << (347 / 10) % 10
              << ", loose = " << 347 % 10 << "\n";
    std::cout << "347 beans in cups of 10: full cups = " << 347 / 10 << ", left over = " << 347 % 10
              << ", cups needed for all = " << (347 + 10 - 1) / 10 << "\n";
    std::cout << "digit 7 as ones, tens, hundreds, thousands: " << 7 * 1 << " " << 7 * 10 << " "
              << 7 * 100 << " " << 7 * 1000 << "\n";
    std::cout << "a thousand thousands = " << 1000 * 1000 << "\n";
    std::cout << "a thousand millions = " << 1000LL * 1000 * 1000 << "\n";
    long long s = 1000000;
    std::cout << "one million seconds = " << s / 86400 << " days " << (s % 86400) / 3600
              << " hours " << (s % 3600) / 60 << " minutes " << s % 60 << " seconds\n";
    std::cout << "4099 < 4100 is " << (4099 < 4100 ? "true" : "false") << "\n";
    std::cout << "4100 - 4099 = " << 4100 - 4099 << "\n";
    std::cout << "406 = " << 4 * 100 << " + " << 0 * 10 << " + " << 6 << " = " << 4 * 100 + 0 * 10 + 6 << "\n";
    std::cout << "46 = " << 4 * 10 << " + " << 6 << " = " << 4 * 10 + 6 << "\n";
    std::cout << "2,050,300 = " << 2 * 1000000 + 5 * 10000 + 3 * 100 << "\n";
    std::cout << "ordering: 980 < 1,004 < 1,040 < 1,400 is "
              << ((980 < 1004 && 1004 < 1040 && 1040 < 1400) ? "true" : "false") << "\n";
    std::cout << "1,000,000 / 4 friends = " << 1000000 / 4 << " each\n";
    std::cout << "seconds in a day: 24 x 60 x 60 = " << 24 * 60 * 60 << "\n";
    std::cout << "1000000 / 86400 = " << 1000000 / 86400 << " r " << 1000000 % 86400 << "\n";
    std::cout << "49600 / 3600 = " << 49600 / 3600 << " (" << 13 * 3600 << " s) r " << 49600 % 3600 << "\n";
    std::cout << "2800 / 60 = " << 2800 / 60 << " (" << 46 * 60 << " s) r " << 2800 % 60 << "\n";
    std::cout << "340 tens = " << 340 * 10 << "; 3400 + 7 = " << 3400 + 7 << "; 34 + 1 = " << 34 + 1 << "\n";
    std::cout << "with tens = 5: " << 3 * 1000 + 4 * 100 + 5 * 10 + 7 << "\n";
    std::cout << "4050 = " << 4000 + 0 + 50 + 0 << "; digit 6 in 5628 is worth " << (5628 / 100) % 10 * 100 << "\n";
    std::cout << "7099 < 7190 is " << (7099 < 7190 ? "true" : "false") << "\n";
    std::cout << "128 marbles in boxes of 10: full = " << 128 / 10 << ", left = " << 128 % 10
              << ", boxes needed = " << (128 + 10 - 1) / 10 << "\n";
    std::cout << "340 beans in cups of 10: cups needed = " << (340 + 10 - 1) / 10 << "\n";
    return 0;
}
