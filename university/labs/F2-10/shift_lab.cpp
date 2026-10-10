#include <iostream>
#include <string>
#include <vector>

// Lab reference solution: a shift log written by constructors and destructors.
// Every cook who starts a shift must also end it, even if the block is left early.
std::vector<std::string> shift_log;

class Shift
{
public:
    explicit Shift(std::string cook) : cook_(cook)
    {
        shift_log.push_back("start " + cook_);
    }

    ~Shift()
    {
        shift_log.push_back("end " + cook_);
    }

    Shift(const Shift&) = delete;
    Shift& operator=(const Shift&) = delete;

private:
    std::string cook_;
};

// Leaves early (return) when there are no orders: the destructor must still run.
int cook_orders(const std::string& cook, int orders)
{
    Shift s(cook);
    if (orders == 0) {
        return 0;
    }
    return orders * 2;  // minutes of work, say
}

int main()
{
    int minutes = cook_orders("Amara", 3);   // two statements: the order of calls is fixed
    minutes = minutes + cook_orders("Jonas", 0);
    {
        Shift a("Mei");
        Shift b("Tariq");
    }
    for (const std::string& line : shift_log) {
        std::cout << line << '\n';
    }
    bool ok = shift_log.size() == 8 && shift_log[2] == "start Jonas" && shift_log[3] == "end Jonas"
              && shift_log[6] == "end Tariq" && shift_log[7] == "end Mei";
    std::cout << "minutes = " << minutes << "; shift log test: " << (ok ? "pass" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
