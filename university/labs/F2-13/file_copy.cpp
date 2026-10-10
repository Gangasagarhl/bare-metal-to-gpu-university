#include <fcntl.h>
#include <unistd.h>

#include <string>

class File
{
public:
    File(const std::string& path, int flags) : fd_(::open(path.c_str(), flags, 0644)) {}
    ~File()
    {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }
    File(const File&) = delete;
    File& operator=(const File&) = delete;

private:
    int fd_;
};

int main()
{
    File a("orders.txt", O_RDONLY);
    File b = a;   // two owners of one file descriptor would close it twice
    return 0;
}
