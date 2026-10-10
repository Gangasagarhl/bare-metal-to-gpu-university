// urtos.h - F3-40 Listing 1: uRTOS version 2 = F3-39's kernel plus two kernel objects that
// every RTOS offers in some form: a mutex (optionally with priority inheritance) and a
// fixed-size message queue. Changes from F3-39 are marked "v2".
#pragma once
#include <stdint.h>

namespace os {

using TaskFunction = void (*)();

enum class State : uint8_t { Ready, Sleeping, Blocked };   // v2: Blocked

struct Task {
    uint32_t* sp;            // saved stack pointer: MUST stay the first member (used by asm)
    const char* name;
    uint8_t priority;        // larger number = more urgent; 0 is reserved for idle
    State state;
    uint32_t wakeTick;       // when Sleeping: the tick at which the task becomes Ready
    uint32_t releaseTime;    // time of the latest release, in processor clocks
    char letter;             // one character for the timeline
    uint8_t basePriority;    // v2: priority without inheritance
    const void* waitingOn;   // v2: the mutex or queue a Blocked task waits for
};

struct Switch {              // one entry of the context-switch log
    uint32_t time;           // processor clocks since start
    char to;                 // letter of the task that starts running
};

// Create a task with its own stack (an array the caller owns). Call before start().
Task& createTask(TaskFunction fn, uint8_t priority, uint32_t* stack, uint32_t words,
                 const char* name, char letter);

// Start scheduling with a tick every tickReload + 1 processor clocks. At stopAtTick the tick
// stops; when every task sleeps after that, the idle task calls finished() once.
[[noreturn]] void start(uint32_t tickReload, uint32_t stopAtTick, void (*finished)());

uint32_t now();              // processor clocks since start (from SysTick)
uint32_t tick();             // SysTick ticks since start
Task& current();
void sleepUntil(uint32_t tick);   // block the calling task until that tick

// v2: a mutex. lock() blocks while another task owns it. With inheritance on, the owner runs
// at the priority of the most urgent waiter until it unlocks.
class Mutex {
public:
    explicit Mutex(bool inheritance) : inheritance_(inheritance) {}
    void lock();
    void unlock();
private:
    bool inheritance_;
    Task* owner_ = nullptr;
};

// v2: a message queue of N values of type uint32_t. send() never blocks (it fails when the
// queue is full); receive() blocks the caller until a message arrives.
class Queue {
public:
    bool send(uint32_t v);
    uint32_t receive();
    bool tryReceive(uint32_t& v);
private:
    static constexpr uint32_t kSize = 8;
    uint32_t buf_[kSize] = {};
    uint32_t head_ = 0, tail_ = 0;
};

// Context-switch log (filled by the scheduler) and its length.
extern Switch switchLog[];
extern uint32_t switchCount;
inline constexpr uint32_t kSwitchLogSize = 256;

}  // namespace os
