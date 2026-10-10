#include <iostream>
#include <string>

void add_salt(std::string& soup)
{
    soup += " + salt";
}

int main()
{
    add_salt("tomato soup");             // a literal is not a variable
    return 0;
}
