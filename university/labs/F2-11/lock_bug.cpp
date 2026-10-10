#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <string>

// Forensic evidence: the menu editor. Only one editor may change the menu at a time,
// so each update takes an exclusive lock on menu.lock first.
bool update_menu(const std::string& dish)
{
    int fd = ::open("menu.lock", O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        std::cout << "  cannot open menu.lock\n";
        return false;
    }
    if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        std::cout << "  menu is locked by someone else, giving up on '" << dish << "'\n";
        ::close(fd);
        return false;
    }
    if (dish == "same as yesterday") {
        std::cout << "  nothing to change\n";
        return true;  // added in a hurry last week
    }
    std::cout << "  menu updated: " << dish << '\n';
    ::flock(fd, LOCK_UN);
    ::close(fd);
    return true;
}

// Prints every open file of this process (Linux: the links in /proc/self/fd).
void list_open_files()
{
    for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
        std::error_code ec;
        std::string target = std::filesystem::read_symlink(entry.path(), ec).string();
        if (target.rfind("/proc/", 0) == 0) {
            target = "(the folder listing used by this function)";
        }
        std::cout << "  fd " << entry.path().filename().string() << " -> " << target << '\n';
    }
}

int main()
{
    const std::string requests[] = {"lentil soup", "same as yesterday", "rice and beans", "tea"};
    for (const std::string& r : requests) {
        std::cout << "request: " << r << '\n';
        update_menu(r);
    }
    std::cout << "open files at the end:\n";
    list_open_files();
    std::filesystem::remove("menu.lock");
    return 0;
}
