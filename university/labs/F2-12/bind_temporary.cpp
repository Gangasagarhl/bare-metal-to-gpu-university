void add_portions(int& portions)
{
    portions = portions + 1;
}

int main()
{
    add_portions(5);   // a temporary cannot bind to a non-const reference
    return 0;
}
