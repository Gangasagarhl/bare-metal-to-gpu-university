// Forensic evidence generator: replays the switch commands a robot program sent
// to its (simulated) motor driver, one line per command, and flags each result.
#include <iostream>
#include <string>

int main()
{
    std::string time;
    std::string wish;
    int s1 = 0;
    int s2 = 0;
    int s3 = 0;
    int s4 = 0;

    std::cout << "time   wish      S1 S2 S3 S4  result\n";
    while (std::cin >> time >> wish >> s1 >> s2 >> s3 >> s4) {
        std::string result = "motor not driven";
        if ((s1 == 1 && s2 == 1) || (s3 == 1 && s4 == 1)) {
            result = "SHORT CIRCUIT";
        } else if (s1 == 1 && s4 == 1) {
            result = "forward";
        } else if (s3 == 1 && s2 == 1) {
            result = "backward";
        }
        std::cout << time << "  " << wish << "  " << s1 << "  " << s2 << "  " << s3 << "  "
                  << s4 << "   " << result << "\n";
    }
    return 0;
}
