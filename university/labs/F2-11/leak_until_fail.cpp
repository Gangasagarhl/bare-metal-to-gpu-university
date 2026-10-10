#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

// How long can a program leak file descriptors before open() fails?
// To get the same answer on every machine, we first lower this process's limit to 32 open files.
int main()
{
    rlimit limit{};
    limit.rlim_cur = 32;
    limit.rlim_max = 32;
    if (::setrlimit(RLIMIT_NOFILE, &limit) != 0) {
        std::cout << "setrlimit failed: " << std::strerror(errno) << '\n';
        return 1;
    }
    std::vector<int> leaked;
    leaked.reserve(64);
    std::string reason;
    while (true) {
        int fd = ::open("leak_test.txt", O_WRONLY | O_CREAT, 0644);
        if (fd < 0) {
            reason = std::strerror(errno);
            break;
        }
        leaked.push_back(fd);  // "forgotten": not closed until the end of the experiment
    }
    for (int fd : leaked) {
        ::close(fd);  // clean up so the program (and its sanitizers) can work normally again
    }
    std::cout << "open failed after " << leaked.size() << " leaked files: " << reason << '\n';
    std::cout << "highest descriptor number handed out: " << leaked.back() << '\n';
    std::filesystem::remove("leak_test.txt");
    return 0;
}
