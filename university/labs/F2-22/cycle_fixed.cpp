#include <iostream>
#include <memory>
#include <string>

struct Cook
{
    std::string name;
    std::weak_ptr<Cook> partner;        // observing link: breaks the cycle
    explicit Cook(std::string n) : name(std::move(n)) {}
    ~Cook() { std::cout << "  " << name << " goes home\n"; }
};

int main()
{
    {
        auto ana = std::make_shared<Cook>("Ana");
        auto bo = std::make_shared<Cook>("Bo");
        ana->partner = bo;
        bo->partner = ana;
        std::cout << "owners of Ana: " << ana.use_count() << ", of Bo: " << bo.use_count() << '\n';
        if (auto p = ana->partner.lock()) {
            std::cout << "Ana's partner is " << p->name << '\n';
        }
    }
    std::cout << "end of the shift\n";
    return 0;
}
