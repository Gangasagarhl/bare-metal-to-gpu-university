// Forensic program (F2-40): "The deadlock at 3 a.m.". A small ledger service.
// run.sh starts it, waits, attaches gdb and dumps the stacks of all threads.
#include <pthread.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

struct Account
{
    std::string name;
    long balance = 0;
    std::mutex m;
};

void writeAuditLine(const std::string& line)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // the audit disk is slow at night
    (void)line;
}

void transfer(Account& from, Account& to, long amount)
{
    std::lock_guard<std::mutex> lockFrom(from.m);
    writeAuditLine("transfer from " + from.name);
    std::lock_guard<std::mutex> lockTo(to.m);
    from.balance -= amount;
    to.balance += amount;
}

int main()
{
    Account savings{"savings", 500, {}};
    Account kitchen{"kitchen", 200, {}};
    std::cout << "ledger: nightly jobs starting" << std::endl;
    std::thread payroll([&] {
        pthread_setname_np(pthread_self(), "payroll");
        transfer(savings, kitchen, 100);
    });
    std::thread refunds([&] {
        pthread_setname_np(pthread_self(), "refunds");
        transfer(kitchen, savings, 30);
    });
    payroll.join();
    refunds.join();
    std::cout << "ledger: nightly jobs done" << std::endl;
    return 0;
}
