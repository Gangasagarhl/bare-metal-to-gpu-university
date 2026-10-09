#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <string>

// RAII owner of an exclusive lock on a lock file.
// Invariant: if locked() is true, this object holds the lock and fd_ is open.
class MenuLock
{
public:
    explicit MenuLock(const std::string& path) : fd_(::open(path.c_str(), O_RDWR | O_CREAT, 0644))
    {
        if (fd_ >= 0 && ::flock(fd_, LOCK_EX | LOCK_NB) == 0) {
            locked_ = true;
        }
    }

    ~MenuLock()
    {
        if (locked_) {
            ::flock(fd_, LOCK_UN);
        }
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    MenuLock(const MenuLock&) = delete;
    MenuLock& operator=(const MenuLock&) = delete;

    bool locked() const { return locked_; }

private:
    int fd_;
    bool locked_ = false;
};

bool update_menu(const std::string& dish)
{
    MenuLock lock("menu.lock");
    if (!lock.locked()) {
        std::cout << "  menu is locked by someone else, giving up on '" << dish << "'\n";
        return false;
    }
    if (dish == "same as yesterday") {
        std::cout << "  nothing to change\n";
        return true;  // the destructor of lock unlocks and closes
    }
    std::cout << "  menu updated: " << dish << '\n';
    return true;
}

int main()
{
    const std::string requests[] = {"lentil soup", "same as yesterday", "rice and beans", "tea"};
    int updated = 0;
    for (const std::string& r : requests) {
        std::cout << "request: " << r << '\n';
        if (update_menu(r)) {
            updated = updated + 1;
        }
    }
    std::cout << "requests handled: " << updated << " of 4\n";
    std::filesystem::remove("menu.lock");
    return updated == 4 ? 0 : 1;
}
