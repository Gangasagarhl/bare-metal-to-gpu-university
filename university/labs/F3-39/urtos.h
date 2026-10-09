// urtos.h - F3-39 Listing 1: the interface of uRTOS, the university's teaching RTOS for the
// lab microcontroller (Armv7-M). Fixed-priority preemptive scheduling, a periodic tick,
// sleeping until a tick, and a log of every context switch.
#pragma once
#include <stdint.h>

namespace os {

using TaskFunction = void (*)();

enum class State : uint8_t { Ready, Sleeping };

struct Task {
    uint32_t* sp;            // saved stack pointer: MUST stay the first member (used by asm)
    const char* name;
    uint8_t priority;        // larger number = more urgent; 0 is reserved for idle
    State state;
    uint32_t wakeTick;       // when Sleeping: the tick at which the task becomes Ready
    uint32_t releaseTime;    // time of the latest release, in processor clocks
    char letter;             // one character for the timeline
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

// Context-switch log (filled by the scheduler) and its length.
extern Switch switchLog[];
extern uint32_t switchCount;
inline constexpr uint32_t kSwitchLogSize = 256;

}  // namespace os
