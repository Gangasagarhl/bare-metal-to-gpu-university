#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Rule of zero: the members already copy and move correctly, so we write none of the five.
class Recipe
{
public:
    Recipe(std::string name, std::vector<std::string> steps)
        : name_(std::move(name)), steps_(std::move(steps))
    {
    }

    void add_step(const std::string& step) { steps_.push_back(step); }
    std::size_t steps() const { return steps_.size(); }
    const std::string& name() const { return name_; }

private:
    std::string name_;
    std::vector<std::string> steps_;
};

int main()
{
    Recipe soup("lentil soup", {"wash lentils", "boil", "season"});
    Recipe spicy = soup;              // a deep copy: its own name and its own steps
    spicy.add_step("add chilli");
    std::cout << soup.name() << ": " << soup.steps() << " steps\n";
    std::cout << spicy.name() << " (copy): " << spicy.steps() << " steps\n";

    Recipe moved = std::move(spicy);  // the steps are handed over, not copied
    std::cout << "moved: " << moved.steps() << " steps\n";
    return 0;
}
