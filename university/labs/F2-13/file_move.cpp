#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

int open_file_count()
{
    int n = 0;
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
        (void)entry;
        n = n + 1;
    }
    return n;
}

// The RAII File of F2-11, now movable. Invariant: fd_ >= 0 owns a file; fd_ == -1 owns nothing.
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
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    File(File&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

    File& operator=(File&& other) noexcept
    {
        if (this != &other) {
            if (fd_ >= 0) {
                ::close(fd_);
            }
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    bool owns_file() const { return fd_ >= 0; }

private:
    int fd_;
};

File open_log(const std::string& name)
{
    return File(name, O_WRONLY | O_CREAT | O_TRUNC);
}

int main()
{
    const int at_start = open_file_count();
    {
        std::vector<File> logs;
        for (int i = 0; i < 5; ++i) {
            logs.push_back(open_log("station" + std::to_string(i) + ".log"));
        }
        std::cout << "5 logs in a vector: open files = " << open_file_count() - at_start << '\n';

        File first = std::move(logs[0]);
        std::cout << "after moving logs[0] out: logs[0] owns a file: " << logs[0].owns_file()
                  << ", first owns a file: " << first.owns_file()
                  << ", open files = " << open_file_count() - at_start << '\n';

        first = open_log("station9.log");   // move assignment closes the old file first
        std::cout << "after first = open_log(...): open files = "
                  << open_file_count() - at_start << '\n';
    }
    std::cout << "after the block: open files = " << open_file_count() - at_start << '\n';
    for (int i : {0, 1, 2, 3, 4, 9}) {
        std::filesystem::remove("station" + std::to_string(i) + ".log");
    }
    return 0;
}
