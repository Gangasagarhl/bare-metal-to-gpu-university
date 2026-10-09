// The initializer list is written in a different order from the member declarations.
class Shelf
{
public:
    explicit Shelf(int capacity) : capacity_(capacity), free_(capacity_) {}
    int free() const { return free_; }

private:
    int free_;      // declared first: initialised first, from capacity_ (not yet set!)
    int capacity_;
};

int main()
{
    Shelf s(12);
    return s.free();
}
