// atomic_replace.cc - F3-43: replace a small file so that after a crash you find either the
// old contents or the new contents, never an empty or half-written file.
//   atomic_replace safe   <dir>   write temp, fsync, rename over, fsync the directory
//   atomic_replace unsafe <dir>   truncate the file in place and write (the common bug)
#include <cstdio>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <unistd.h>

class Fd
{
public:
    explicit Fd(int fd) : fd_(fd) {}
    ~Fd() { if (fd_ >= 0) { ::close(fd_); } }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    int get() const { return fd_; }
private:
    int fd_;
};

bool fail(const char* what)
{
    std::perror(what);
    return false;
}

bool writeAll(int fd, const std::string& s)
{
    std::size_t done = 0;
    while (done < s.size()) {
        ssize_t n = ::write(fd, s.data() + done, s.size() - done);
        if (n < 0) {
            return fail("write");
        }
        done += static_cast<std::size_t>(n);
    }
    return true;
}

bool replaceSafely(const std::string& dir, const std::string& text)
{
    const std::string tmp = dir + "/settings.conf.tmp";
    const std::string dst = dir + "/settings.conf";
    {
        Fd f(::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
        if (f.get() < 0) return fail("open tmp");
        if (!writeAll(f.get(), text)) return false;
        if (::fsync(f.get()) != 0) return fail("fsync tmp");    // 1: new data durable
    }
    if (::rename(tmp.c_str(), dst.c_str()) != 0) return fail("rename");  // 2: atomic switch
    Fd d(::open(dir.c_str(), O_RDONLY | O_DIRECTORY));
    if (d.get() < 0) return fail("open dir");
    if (::fsync(d.get()) != 0) return fail("fsync dir");         // 3: the rename durable
    return true;
}

bool replaceUnsafely(const std::string& dir, const std::string& text)
{
    const std::string dst = dir + "/settings.conf";
    Fd f(::open(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644));   // old contents gone now
    if (f.get() < 0) return fail("open");
    return writeAll(f.get(), text);                                   // no fsync at all
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: atomic_replace safe|unsafe <dir>\n");
        return 2;
    }
    const std::string text = "volume=7\nlanguage=en\n";
    const bool safe = std::strcmp(argv[1], "safe") == 0;
    const bool ok = safe ? replaceSafely(argv[2], text) : replaceUnsafely(argv[2], text);
    std::printf("%s replace: %s\n", safe ? "safe" : "unsafe", ok ? "done" : "FAILED");
    return ok ? 0 : 1;
}
