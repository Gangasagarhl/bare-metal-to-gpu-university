// Listing 1 (F2-39): message passing. The producer writes a plain payload, then raises an
// atomic flag; the consumer waits for the flag, then reads the payload.
// Argument "relaxed" uses memory_order_relaxed for the flag; anything else uses release/acquire.
#include <atomic>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

struct Order
{
    std::string dish;
    int table = 0;
};

Order payload;                   // plain data, NOT atomic
std::atomic<bool> ready{false};  // the flag that publishes it

void produce(std::memory_order storeOrder)
{
    payload.dish = "dumplings";  // (1) write the data
    payload.table = 7;
    ready.store(true, storeOrder);  // (2) publish
}

void consume(std::memory_order loadOrder)
{
    while (!ready.load(loadOrder)) {  // (3) wait until published
    }
    std::cout << "consumer read: " << payload.dish << " for table " << payload.table << '\n';  // (4)
}

int main(int argc, char** argv)
{
    const bool relaxed = argc > 1 && std::strcmp(argv[1], "relaxed") == 0;
    const std::memory_order st = relaxed ? std::memory_order_relaxed : std::memory_order_release;
    const std::memory_order ld = relaxed ? std::memory_order_relaxed : std::memory_order_acquire;
    std::cout << "flag orders: " << (relaxed ? "relaxed / relaxed" : "release / acquire") << '\n';
    std::thread consumer(consume, ld);
    std::thread producer(produce, st);
    producer.join();
    consumer.join();
    return 0;
}
