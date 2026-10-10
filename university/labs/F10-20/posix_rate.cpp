// posix_rate.cpp - a periodic "sensor" thread and a "consumer" thread that use only
// POSIX calls: pthreads with an explicit scheduling policy, clock_nanosleep with an
// absolute release time, a POSIX message queue and a POSIX semaphore.
// The same calls are what a NuttX application would use; this build runs it on Linux.
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

constexpr long kPeriodNs = 4'000'000;   // 4 ms: a 250 Hz loop
constexpr int kSamples = 250;           // one second of samples

struct Sample
{
    std::int32_t seq;
    std::int64_t releaseNs;
};

struct Shared
{
    mqd_t queue;
    sem_t done;
    long maxLatenessNs = 0;
    int lateReleases = 0;          // lateness of more than one period
    int received = 0;
    int gaps = 0;
};

std::int64_t toNs(const timespec& t)
{
    return static_cast<std::int64_t>(t.tv_sec) * 1'000'000'000 + t.tv_nsec;
}

void addNs(timespec& t, long ns)
{
    t.tv_nsec += ns;
    while (t.tv_nsec >= 1'000'000'000) {
        t.tv_nsec -= 1'000'000'000;
        ++t.tv_sec;
    }
}

void* sensorTask(void* arg)
{
    auto* s = static_cast<Shared*>(arg);
    timespec release{};
    clock_gettime(CLOCK_MONOTONIC, &release);
    for (int i = 0; i < kSamples; ++i) {
        addNs(release, kPeriodNs);
        // Absolute sleep: a late wake-up does not shift every later release.
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &release, nullptr) == EINTR) {
        }
        timespec now{};
        clock_gettime(CLOCK_MONOTONIC, &now);
        const long late = static_cast<long>(toNs(now) - toNs(release));
        if (late > s->maxLatenessNs) {
            s->maxLatenessNs = late;
        }
        if (late > kPeriodNs) {
            ++s->lateReleases;
        }
        const Sample msg{i, toNs(release)};
        mq_send(s->queue, reinterpret_cast<const char*>(&msg), sizeof msg, 0);
    }
    const Sample stop{-1, 0};
    mq_send(s->queue, reinterpret_cast<const char*>(&stop), sizeof stop, 0);
    return nullptr;
}

void* consumerTask(void* arg)
{
    auto* s = static_cast<Shared*>(arg);
    std::int32_t expected = 0;
    for (;;) {
        Sample msg{};
        const ssize_t n = mq_receive(s->queue, reinterpret_cast<char*>(&msg), sizeof msg, nullptr);
        if (n != static_cast<ssize_t>(sizeof msg) || msg.seq < 0) {
            break;
        }
        if (msg.seq != expected) {
            ++s->gaps;
        }
        expected = msg.seq + 1;
        ++s->received;
    }
    sem_post(&s->done);
    return nullptr;
}

// Create a thread with an explicit policy and priority; fall back to the default
// policy if the system refuses (for example without the needed privilege).
int startTask(pthread_t& t, void* (*fn)(void*), void* arg, int priority, std::string& how)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched_param p{};
    p.sched_priority = priority;
    pthread_attr_setschedparam(&attr, &p);
    int rc = pthread_create(&t, &attr, fn, arg);
    pthread_attr_destroy(&attr);
    if (rc == 0) {
        how = "SCHED_FIFO priority " + std::to_string(priority);
        return 0;
    }
    how = std::string("default policy (SCHED_FIFO refused: ") + std::strerror(rc) + ")";
    return pthread_create(&t, nullptr, fn, arg);
}

} // namespace

int main()
{
    Shared s{};
    const std::string name = "/dn302_f10_20_" + std::to_string(getpid());
    mq_attr qa{};
    qa.mq_maxmsg = 8;
    qa.mq_msgsize = sizeof(Sample);
    s.queue = mq_open(name.c_str(), O_CREAT | O_RDWR, 0600, &qa);
    if (s.queue == static_cast<mqd_t>(-1)) {
        std::printf("mq_open failed: %s\n", std::strerror(errno));
        return 1;
    }
    sem_init(&s.done, 0, 0);

    std::printf("sched_get_priority_min/max(SCHED_FIFO) = %d / %d\n",
                sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO));
    pthread_t consumer{};
    pthread_t sensor{};
    std::string howC;
    std::string howS;
    if (startTask(consumer, consumerTask, &s, 50, howC) != 0 ||
        startTask(sensor, sensorTask, &s, 60, howS) != 0) {
        std::printf("pthread_create failed\n");
        return 1;
    }
    std::printf("consumer task: %s\n", howC.c_str());
    std::printf("sensor task:   %s\n", howS.c_str());

    sem_wait(&s.done);   // the consumer posts when the stop message arrives
    pthread_join(sensor, nullptr);
    pthread_join(consumer, nullptr);
    mq_close(s.queue);
    mq_unlink(name.c_str());
    sem_destroy(&s.done);

    std::printf("period %ld us, %d releases, %d messages received, %d sequence gaps\n",
                kPeriodNs / 1000, kSamples, s.received, s.gaps);
    std::printf("worst lateness of a release: %ld us (measured; varies from run to run)\n",
                s.maxLatenessNs / 1000);
    std::printf("releases later than one period: %d\n", s.lateReleases);
    const bool ok = s.received == kSamples && s.gaps == 0;
    std::printf("check: every sample delivered in order: %s\n", ok ? "yes" : "NO");
    return ok ? 0 : 1;
}
