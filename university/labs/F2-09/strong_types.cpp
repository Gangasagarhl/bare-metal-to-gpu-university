// Two small "strong types": both hold an int, but the compiler keeps them apart.
class Portions
{
public:
    explicit Portions(int n) : n_(n) {}
    int value() const { return n_; }

private:
    int n_;
};

class Grams
{
public:
    explicit Grams(int g) : g_(g) {}
    int value() const { return g_; }

private:
    int g_;
};

int serve(Portions p)
{
    return p.value();
}

int main()
{
    Grams flour(500);
    return serve(flour);  // a mix-up: grams passed where portions are expected
}
