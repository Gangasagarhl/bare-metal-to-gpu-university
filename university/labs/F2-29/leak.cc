// leak.cc: two shared_ptr objects that own each other are never destroyed.
#include <cstdio>
#include <memory>
#include <string>

struct Person
{
    std::string name;
    std::shared_ptr<Person> friend_of;        // a strong (owning) pointer: the bug
};

int main()
{
    auto ada = std::make_shared<Person>();
    auto bo = std::make_shared<Person>();
    ada->name = "Ada";
    bo->name = "Bo";
    ada->friend_of = bo;
    bo->friend_of = ada;                      // a cycle: each keeps the other alive
    std::printf("%s and %s are friends\n", ada->name.c_str(), bo->name.c_str());
    return 0;
}
