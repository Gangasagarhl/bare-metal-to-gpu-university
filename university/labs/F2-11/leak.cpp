#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <string>

// Counts the files this process has open right now (Linux: entries of /proc/self/fd).
int open_file_count()
{
    int n = 0;
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
        (void)entry;
        n = n + 1;
    }
    return n;
}

// Appends one order line to orders.txt. Returns false for an empty order.
bool append_order(const std::string& order)
{
    int fd = ::open("orders.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        return false;
    }
    if (order.empty()) {
        return false;  // early return: the close() below is never reached
    }
    std::string line = order + "\n";
    ssize_t written = ::write(fd, line.data(), line.size());
    ::close(fd);
    return written == static_cast<ssize_t>(line.size());
}

int main()
{
    std::cout << "open files at start: " << open_file_count() << '\n';
    const std::string orders[] = {"rice", "", "soup", "", "", "tea"};
    for (const std::string& o : orders) {
        append_order(o);
    }
    std::cout << "open files after 6 orders (3 empty): " << open_file_count() << '\n';
    std::filesystem::remove("orders.txt");
    return 0;
}
