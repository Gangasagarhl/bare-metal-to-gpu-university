#include <string>

const std::string& todays_special()
{
    std::string special = "lentil soup";
    return special;   // returns a reference to a local that dies at the closing brace
}

int main()
{
    return static_cast<int>(todays_special().size());
}
