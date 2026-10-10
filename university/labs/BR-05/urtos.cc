// BR-05 lab: copied unchanged from university/labs/F3-40/urtos.cc (only this line added).
// urtos.cc - F3-40 Listing 2: uRTOS version 2 (F3-39's kernel plus Mutex and Queue, "v2"). Scheduling rule: the highest-priority Ready
// task runs; a task runs until it sleeps or a more urgent task becomes Ready (preemption).
// Context switching uses the Armv7-M PendSV exception and the process stack pointer (PSP);
// the hardware saves r0-r3, r12, lr, pc, xPSR on exception entry and we save r4-r11
// (Armv7-M Architecture Reference Manual, exception entry and return; pending verification).
#include "urtos.h"

#include "board.h"

namespace os {

namespace {

constexpr uint32_t kMaxTasks = 6;
Task tasks[kMaxTasks];
uint32_t taskCount = 0;
volatile uint32_t ticks = 0;
uint32_t tickStamp[64];               // time of the last 64 ticks
uint32_t reloadValue = 0;
uint32_t stopTick = 0;
uint32_t idleStack[128];
void (*onFinished)() = nullptr;

constexpr uintptr_t kIcsr = 0xE000ED04;        // interrupt control and state
constexpr uintptr_t kShpr3 = 0xE000ED20;       // system handler priorities (PendSV, SysTick)

void pendSwitch() { board::reg(kIcsr) = 1u << 28; }   // PENDSVSET: switch at the next chance

void idle()
{
    for (;;) {
        if (ticks >= stopTick && onFinished != nullptr) {
            void (*f)() = onFinished;
            onFinished = nullptr;
            f();                                      // every other task sleeps for good
        }
        asm volatile("wfi");
    }
}

[[noreturn]] void taskReturned()
{
    board::print("a task function returned: halting\n");
    board::exitEmulator(false);
}

}  // namespace

Switch switchLog[kSwitchLogSize];
uint32_t switchCount = 0;
Task* currentTask = nullptr;          // read by the assembly below

// Processor clocks since start: whole ticks plus the part of the current tick that SysTick
// has counted. SysTick counts reload, ..., 1, 0 and the tick interrupt is raised as it
// reaches 0, so CVR == 0 means "a tick boundary". If the counter has reached 0 (or already
// reloaded) but the handler has not run yet because interrupts are off, the tick is pending
// (ICSR bit 26, PENDSTSET) and must be counted here.
uint32_t now()
{
    uint32_t primask;
    asm volatile("mrs %0, primask\n cpsid i" : "=r"(primask) :: "memory");
    uint32_t t = ticks;
    const uint32_t cvr = board::reg(board::kSystCvr);
    if ((board::reg(kIcsr) & (1u << 26)) != 0 && (cvr == 0 || cvr > reloadValue / 2)) {
        t += 1;                                        // boundary passed, handler pending
    }
    asm volatile("msr primask, %0" :: "r"(primask) : "memory");
    const uint32_t inTick = cvr == 0 ? 0 : reloadValue + 1 - cvr;
    return t * (reloadValue + 1) + inTick;
}

uint32_t tick() { return ticks; }
Task& current() { return *currentTask; }

Task& createTask(TaskFunction fn, uint8_t priority, uint32_t* stack, uint32_t words,
                 const char* name, char letter)
{
    Task& t = tasks[taskCount++];
    uint32_t* sp = stack + words;                     // stacks grow down
    sp = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(sp) & ~uintptr_t{7});
    // The frame an exception return will unstack: xPSR, PC, LR, r12, r3, r2, r1, r0 ...
    *--sp = 0x01000000u;                               // xPSR: Thumb state bit set
    *--sp = reinterpret_cast<uintptr_t>(fn) & ~1u;     // PC: where the task starts
    *--sp = reinterpret_cast<uintptr_t>(&taskReturned);// LR: if the task function returns
    for (int i = 0; i < 5; ++i) {
        *--sp = 0;                                     // r12, r3, r2, r1, r0
    }
    for (int i = 0; i < 8; ++i) {
        *--sp = 0;                                     // r11 ... r4, restored by our code
    }
    t.sp = sp;
    t.name = name;
    t.priority = priority;
    t.state = State::Ready;
    t.wakeTick = 0;
    t.releaseTime = 0;
    t.letter = letter;
    t.basePriority = priority;
    t.waitingOn = nullptr;
    return t;
}

void sleepUntil(uint32_t wake)
{
    asm volatile("cpsid i" ::: "memory");
    if (wake <= ticks) {                              // overrun: that release is already due
        currentTask->releaseTime = tickStamp[wake % 64];
        asm volatile("cpsie i" ::: "memory");
        return;
    }
    currentTask->wakeTick = wake;
    currentTask->state = State::Sleeping;
    pendSwitch();
    asm volatile("cpsie i" ::: "memory");             // PendSV is taken here
}

namespace {

// v2: wake the most urgent task blocked on `object`; returns it (or nullptr).
Task* wakeOne(const void* object)
{
    Task* best = nullptr;
    for (uint32_t i = 0; i < taskCount; ++i) {
        Task& t = tasks[i];
        if (t.state == State::Blocked && t.waitingOn == object &&
            (best == nullptr || t.priority > best->priority)) {
            best = &t;
        }
    }
    if (best != nullptr) {
        best->state = State::Ready;
        best->waitingOn = nullptr;
    }
    return best;
}

void blockOn(const void* object)          // interrupts must be disabled by the caller
{
    currentTask->state = State::Blocked;
    currentTask->waitingOn = object;
    pendSwitch();
}

uint32_t irqOff()
{
    uint32_t primask;
    asm volatile("mrs %0, primask\n cpsid i" : "=r"(primask) :: "memory");
    return primask;
}

void irqRestore(uint32_t primask) { asm volatile("msr primask, %0" :: "r"(primask) : "memory"); }

}  // namespace

void Mutex::lock()
{
    for (;;) {
        const uint32_t m = irqOff();
        if (owner_ == nullptr) {
            owner_ = currentTask;
            irqRestore(m);
            return;
        }
        if (inheritance_ && owner_->priority < currentTask->priority) {
            owner_->priority = currentTask->priority;   // the owner borrows our urgency
        }
        blockOn(this);
        irqRestore(m);                                  // the switch happens here; retry after
    }
}

void Mutex::unlock()
{
    const uint32_t m = irqOff();
    owner_->priority = owner_->basePriority;            // give back any borrowed priority
    owner_ = nullptr;
    wakeOne(this);
    pendSwitch();                                       // a more urgent task may now run
    irqRestore(m);
}

bool Queue::send(uint32_t v)
{
    const uint32_t m = irqOff();
    if (head_ - tail_ == kSize) {
        irqRestore(m);
        return false;
    }
    buf_[head_ % kSize] = v;
    head_ = head_ + 1;
    if (wakeOne(this) != nullptr) {
        pendSwitch();
    }
    irqRestore(m);
    return true;
}

bool Queue::tryReceive(uint32_t& v)
{
    const uint32_t m = irqOff();
    const bool ok = head_ != tail_;
    if (ok) {
        v = buf_[tail_ % kSize];
        tail_ = tail_ + 1;
    }
    irqRestore(m);
    return ok;
}

uint32_t Queue::receive()
{
    for (;;) {
        uint32_t v = 0;
        if (tryReceive(v)) {
            return v;
        }
        const uint32_t m = irqOff();
        if (head_ == tail_) {
            blockOn(this);
        }
        irqRestore(m);
    }
}

// Called by PendSV with interrupts disabled: choose the next task to run.
extern "C" void os_selectNext()
{
    Task* best = nullptr;
    for (uint32_t i = 0; i < taskCount; ++i) {
        Task& t = tasks[i];
        if (t.state == State::Ready && (best == nullptr || t.priority > best->priority)) {
            best = &t;
        }
    }
    if (best != currentTask && switchCount < kSwitchLogSize) {
        switchLog[switchCount++] = Switch{now(), best->letter};
    }
    currentTask = best;
}

extern "C" void SysTick_Handler()
{
    const uint32_t t = ticks + 1;
    ticks = t;
    tickStamp[t % 64] = now();
    if (t >= stopTick) {
        board::reg(board::kSystCsr) = 0;               // stop the tick: the demo is over
    }
    bool wake = false;
    for (uint32_t i = 0; i < taskCount; ++i) {
        Task& task = tasks[i];
        if (task.state == State::Sleeping && task.wakeTick <= t && t < stopTick) {
            task.state = State::Ready;
            task.releaseTime = now();
            wake = wake || task.priority > currentTask->priority;
        }
    }
    if (wake) {
        pendSwitch();                                  // preempt the running task
    }
}

// Save r4-r11 of the old task on its stack, remember its SP, pick the next task, restore.
extern "C" __attribute__((naked)) void PendSV_Handler()
{
    asm volatile(
        "cpsid i                \n"
        "mrs   r0, psp          \n"
        "stmdb r0!, {r4-r11}    \n"
        "ldr   r1, =currentTask_ptr \n"
        "ldr   r1, [r1]         \n"   // r1 = &currentTask
        "ldr   r2, [r1]         \n"   // r2 = currentTask
        "str   r0, [r2]         \n"   // currentTask->sp = r0
        "push  {r1, lr}         \n"
        "bl    os_selectNext    \n"
        "pop   {r1, lr}         \n"
        "ldr   r2, [r1]         \n"   // r2 = the new currentTask
        "ldr   r0, [r2]         \n"
        "ldmia r0!, {r4-r11}    \n"
        "msr   psp, r0          \n"
        "cpsie i                \n"
        "bx    lr               \n");
}

// The first switch: start the first task through an exception return to Thread mode, PSP.
extern "C" __attribute__((naked)) void SVC_Handler()
{
    asm volatile(
        "ldr   r1, =currentTask_ptr \n"
        "ldr   r1, [r1]         \n"
        "ldr   r2, [r1]         \n"
        "ldr   r0, [r2]         \n"
        "ldmia r0!, {r4-r11}    \n"
        "msr   psp, r0          \n"
        "ldr   lr, =0xFFFFFFFD  \n"   // EXC_RETURN: Thread mode, use PSP
        "bx    lr               \n");
}

extern "C" Task** const currentTask_ptr = &currentTask;

void start(uint32_t tickReload, uint32_t stopAtTick, void (*finished)())
{
    onFinished = finished;
    createTask(idle, 0, idleStack, 128, "idle", '.');
    stopTick = stopAtTick;
    board::reg(kShpr3) = (0x80u << 24) | (0xFFu << 16);   // SysTick 0x80, PendSV lowest
    reloadValue = tickReload;
    board::reg(board::kSystRvr) = tickReload;
    board::reg(board::kSystCvr) = 0;                     // time 0 (see now())
    currentTask = nullptr;
    os_selectNext();                                     // the most urgent task goes first
    board::reg(board::kSystCsr) = 0x7;                   // processor clock, interrupt, enable
    asm volatile("svc #0");
    for (;;) {
    }
}

}  // namespace os
