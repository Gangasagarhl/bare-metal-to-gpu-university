#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

// Lab reference solution: the RAII File wrapper with reading added, and its tests.
int open_file_count()
{
    int n = 0;
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
        (void)entry;
        n = n + 1;
    }
    return n;
}

class File
{
public:
    File(const std::string& path, int flags) : fd_(::open(path.c_str(), flags, 0644))
    {
        if (fd_ < 0) {
            throw std::runtime_error("File: cannot open " + path);
        }
    }

    ~File() { /* ::close(fd_); */ }  // lab step 4: sabotaged on purpose

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    bool write_text(const std::string& text)
    {
        ssize_t written = ::write(fd_, text.data(), text.size());
        return written == static_cast<ssize_t>(text.size());
    }

    std::string read_all()
    {
        std::string result;
        char buffer[64];
        ssize_t got = ::read(fd_, buffer, sizeof buffer);
        while (got > 0) {
            result.append(buffer, static_cast<std::size_t>(got));
            got = ::read(fd_, buffer, sizeof buffer);
        }
        return result;
    }

private:
    int fd_;
};

int failures = 0;

void expect(bool condition, const char* what)
{
    if (!condition) {
        std::cout << "FAIL: " << what << '\n';
        failures = failures + 1;
    }
}

int main()
{
    const int at_start = open_file_count();
    {
        File out("lab_test.txt", O_WRONLY | O_CREAT | O_TRUNC);
        expect(open_file_count() == at_start + 1, "one more open file while out lives");
        expect(out.write_text("soup\nrice\n"), "write_text writes every byte");
    }
    expect(open_file_count() == at_start, "file closed at the end of the block");
    {
        File in("lab_test.txt", O_RDONLY);
        expect(in.read_all() == "soup\nrice\n", "read_all returns what was written");
    }
    bool thrown = false;
    try {
        File missing("no_such_folder/x.txt", O_RDONLY);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    expect(thrown, "opening a missing file throws");
    for (int i = 0; i < 1000; ++i) {
        File again("lab_test.txt", O_RDONLY);
    }
    expect(open_file_count() == at_start, "1000 opens leave no file open");
    std::filesystem::remove("lab_test.txt");
    std::cout << "File tests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
