#include <iostream>
#include <memory>
#include <string>
#include <utility>

struct Knife
{
    std::string label;
    explicit Knife(std::string l) : label(std::move(l)) { std::cout << "  " << label << " taken from the wall\n"; }
    ~Knife() { std::cout << "  " << label << " returned to the wall\n"; }
};

void useAndReturn(std::unique_ptr<Knife> k)   // takes ownership: k dies at the end
{
    std::cout << "  chopping with " << k->label << '\n';
}

int main()
{
    std::cout << "1. one owner\n";
    auto a = std::make_unique<Knife>("knife A");
    std::cout << "2. move ownership to b\n";
    std::unique_ptr<Knife> b = std::move(a);
    std::cout << "   a is " << (a ? "an owner" : "empty") << ", b holds " << b->label << '\n';
    std::cout << "3. pass ownership into a function\n";
    useAndReturn(std::move(b));
    std::cout << "4. reset replaces the owned object\n";
    auto c = std::make_unique<Knife>("knife C");
    c.reset(new Knife("knife D"));
    std::cout << "5. sizeof(unique_ptr<Knife>) = " << sizeof(c) << ", sizeof(Knife*) = "
              << sizeof(Knife*) << '\n';
    std::cout << "6. end of main\n";
    return 0;
}
