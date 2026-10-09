#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
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

std::mutex menu_mutex;

void write_menu(const std::string& dish)
{
    std::lock_guard<std::mutex> guard(menu_mutex);  // locked here, unlocked at the closing brace
    std::ofstream out("menu.txt");                    // opened here, closed at the closing brace
    out << dish << '\n';
    std::cout << "  inside write_menu: open files = " << open_file_count() << '\n';
}

int main()
{
    std::cout << "before: open files = " << open_file_count() << '\n';
    write_menu("lentil soup");
    std::cout << "after:  open files = " << open_file_count() << '\n';
    std::cout << "mutex free again: " << (menu_mutex.try_lock() ? "yes" : "no") << '\n';
    menu_mutex.unlock();
    std::filesystem::remove("menu.txt");
    return 0;
}
