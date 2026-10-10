#include <iostream>
#include <memory>
#include <string>

struct Cook
{
    std::string name;
    std::shared_ptr<Cook> partner;      // owning link in both directions: a cycle
    explicit Cook(std::string n) : name(std::move(n)) {}
    ~Cook() { std::cout << "  " << name << " goes home\n"; }
};

int main()
{
    std::cout << std::unitbuf;
    {
        auto ana = std::make_shared<Cook>("Ana");
        auto bo = std::make_shared<Cook>("Bo");
        ana->partner = bo;
        bo->partner = ana;
        std::cout << "owners of Ana: " << ana.use_count() << ", of Bo: " << bo.use_count() << '\n';
    }
    std::cout << "end of the shift (nobody went home?)\n";
    return 0;
}
