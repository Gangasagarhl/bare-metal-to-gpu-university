// Forensic program (F2-38): "Sold out, and then some". Eight booking threads share an
// atomic counter of sold seats; the hall has 1000 seats.
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

constexpr int kSeats = 1000;
std::atomic<int> sold{0};

bool bookSeat()
{
    if (sold.load() < kSeats) {      // step 1: is there a seat left?
        std::this_thread::yield();    // (the booking system writes a log line here)
        sold.fetch_add(1);            // step 2: take it
        return true;
    }
    return false;
}

int main()
{
    std::vector<std::thread> booths;
    std::vector<int> booked(8, 0);
    for (int b = 0; b < 8; ++b) {
        booths.emplace_back([&booked, b] {
            for (int i = 0; i < 400; ++i) {
                if (bookSeat()) {
                    ++booked[b];
                }
            }
        });
    }
    for (std::thread& t : booths) {
        t.join();
    }
    int total = 0;
    for (int n : booked) {
        total += n;
    }
    std::cout << "seats in the hall: " << kSeats << ", tickets issued: " << total
              << ", counter: " << sold.load() << '\n';
    return 0;
}
