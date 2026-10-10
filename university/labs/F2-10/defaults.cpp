#include <iostream>
#include <string>

class Order
{
public:
    Order() = default;                       // uses the default member initialisers below
    explicit Order(int table) : table_(table) {}
    Order(int table, std::string dish) : table_(table), dish_(dish) {}

    void print() const
    {
        std::cout << "table " << table_ << ", dish '" << dish_ << "', portions " << portions_
                  << '\n';
    }

private:
    int table_ = 0;          // default member initialisers: no member is ever left unset
    std::string dish_ = "none yet";
    int portions_ = 1;
};

int main()
{
    Order empty;
    Order for_table(7);
    Order full(3, "rice");
    empty.print();
    for_table.print();
    full.print();
    return 0;
}
