#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int open_file_count()
{
    int n = 0;
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
        (void)entry;
        n = n + 1;
    }
    return n;
}

// RAII wrapper: owns one open file descriptor. Invariant: fd_ >= 0 while the object lives.
class File
{
public:
    File(const std::string& path, int flags) : fd_(::open(path.c_str(), flags, 0644))
    {
        if (fd_ < 0) {
            throw std::runtime_error("File: cannot open " + path);
        }
    }

    ~File()
    {
        ::close(fd_);  // runs on every way out of the owner's scope
    }

    File(const File&) = delete;             // one owner only (F2-13 adds moving)
    File& operator=(const File&) = delete;

    bool write_line(const std::string& text)
    {
        std::string line = text + "\n";
        ssize_t written = ::write(fd_, line.data(), line.size());
        return written == static_cast<ssize_t>(line.size());
    }

private:
    int fd_;
};

bool append_order(const std::string& order)
{
    File f("orders.txt", O_WRONLY | O_CREAT | O_APPEND);
    if (order.empty()) {
        return false;  // early return: f's destructor still closes the file
    }
    return f.write_line(order);
}

int main()
{
    std::cout << "open files at start: " << open_file_count() << '\n';
    const std::string orders[] = {"rice", "", "soup", "", "", "tea"};
    for (const std::string& o : orders) {
        append_order(o);
    }
    std::cout << "open files after 6 orders (3 empty): " << open_file_count() << '\n';

    try {
        File missing("no_such_folder/orders.txt", O_RDONLY);
    } catch (const std::runtime_error& e) {
        std::cout << "caught: " << e.what() << '\n';
    }
    std::cout << "open files at the end: " << open_file_count() << '\n';
    std::filesystem::remove("orders.txt");
    return 0;
}
