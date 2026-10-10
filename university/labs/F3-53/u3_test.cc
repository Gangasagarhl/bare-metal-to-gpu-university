// u3_test.cc - F3-53: the U3 acceptance test program. "A test program using std::vector,
// std::map, std::string, iostreams, std::filesystem directory iteration, std::thread with
// std::mutex and std::condition_variable, and exceptions thrown across a shared-library boundary
// produces output identical to the same program on the Linux host." Built by run.sh for three
// architectures; their outputs are compared byte for byte.
#include <algorithm>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

void throw_from_library(int code);                 // in libthrower.so

namespace {

struct Shape
{
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

struct Square : Shape
{
    double side;
    explicit Square(double s) : side(s) {}
    double area() const override { return side * side; }
};

struct Circle : Shape
{
    double r;
    explicit Circle(double radius) : r(radius) {}
    double area() const override { return 3.0 * r * r; }   // a deliberately simple "pi"
};

long sum_with_threads()                            // 4 consumers, 1 producer, numbers 1..1000
{
    std::mutex m;
    std::condition_variable cv;
    std::queue<int> q;
    bool done = false;
    long total = 0;
    std::vector<std::thread> consumers;
    for (int c = 0; c < 4; ++c) {
        consumers.emplace_back([&] {
            long mine = 0;
            for (;;) {
                std::unique_lock<std::mutex> lock(m);
                cv.wait(lock, [&] { return !q.empty() || done; });
                if (q.empty()) break;
                mine += q.front();
                q.pop();
            }
            std::lock_guard<std::mutex> lock(m);
            total += mine;
        });
    }
    for (int i = 1; i <= 1000; ++i) {
        {
            std::lock_guard<std::mutex> lock(m);
            q.push(i);
        }
        cv.notify_one();
    }
    {
        std::lock_guard<std::mutex> lock(m);
        done = true;
    }
    cv.notify_all();
    for (auto& t : consumers) t.join();
    return total;
}

} // namespace

int main(int argc, char** argv)
{
    namespace fs = std::filesystem;
    if (argc != 2) {
        std::cerr << "usage: u3_test <empty scratch folder>\n";
        return 2;
    }

    std::vector<int> v {5, 3, 9, 1, 7};
    std::sort(v.begin(), v.end());
    std::cout << "1 vector sorted:";
    for (int x : v) std::cout << ' ' << x;
    std::cout << '\n';

    std::map<std::string, int> words;
    std::istringstream text("the cat and the hat and the bat");
    for (std::string w; text >> w;) ++words[w];
    std::cout << "2 map word counts:";
    for (const auto& [w, n] : words) std::cout << ' ' << w << '=' << n;
    std::cout << '\n';

    std::cout << "3 iostreams: [" << std::setw(6) << 42 << "] [" << std::left << std::setw(6) << "ab"
              << "] " << std::right << std::fixed << std::setprecision(3) << 2.0 / 3.0 << '\n';

    fs::path dir = argv[1];
    fs::create_directories(dir / "sub");
    std::ofstream(dir / "b.txt") << "bee\n";
    std::ofstream(dir / "a.txt") << "ay\n";
    std::vector<std::string> names;
    for (const auto& e : fs::directory_iterator(dir)) {
        names.push_back(e.path().filename().string() + (e.is_directory() ? "/" : ""));
    }
    std::sort(names.begin(), names.end());
    std::cout << "4 filesystem entries:";
    for (const auto& n : names) std::cout << ' ' << n;
    std::cout << "; size of a.txt = " << fs::file_size(dir / "a.txt") << '\n';
    fs::remove_all(dir);

    std::cout << "5 threads: sum 1..1000 by 4 consumers = " << sum_with_threads() << '\n';

    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Square>(2.0));
    shapes.push_back(std::make_unique<Circle>(1.0));
    int squares = 0;
    for (const auto& s : shapes) {
        if (dynamic_cast<const Square*>(s.get()) != nullptr) ++squares;
    }
    std::cout << "6 RTTI: dynamic_cast found " << squares << " square(s); total area "
              << std::setprecision(1) << shapes[0]->area() + shapes[1]->area() << '\n';

    try {
        throw std::out_of_range("thrown and caught inside the program");
    } catch (const std::logic_error& e) {
        std::cout << "7 exception inside the program: " << e.what() << '\n';
    }
    try {
        throw_from_library(7);
    } catch (const std::runtime_error& e) {
        std::cout << "8 exception across the library boundary: " << e.what() << '\n';
    }
    return 0;
}
