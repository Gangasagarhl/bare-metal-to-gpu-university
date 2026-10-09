class Stock
{
public:
    explicit Stock(int portions) : portions_(portions) {}
    int portions() const { return portions_; }   // const: promises not to change the object
    void take(int n) { portions_ = portions_ - n; }

private:
    int portions_;
};

int show(const Stock& s)
{
    s.take(1);              // error 1: take() is not const
    return s.portions();    // fine: portions() is const
}

int main()
{
    const int tables = 12;
    tables = 13;            // error 2: tables is const
    Stock soup(10);
    return show(soup) + tables;
}
