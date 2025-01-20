#pragma once

#include <thread>

#include <threads.h>
#include <stdint.h>
#include <time.h>


constexpr uint64_t _nanos = 1000'000'000ull;        


/**
 * @brief convert timeval to nanoseconds
 * @param t the timeval
 * @return timeval in nanoseconds
 */
inline uint64_t timeval_nsecs(struct timeval *t)
{
    uint64_t value = (t->tv_sec * _nanos) + (t->tv_usec * 1000);
    return value;
}

inline double ll2d(uint64_t value)
{
    return (double) value / 1e9;
}

static uint64_t gettimex(clock_t id)
{
    struct timespec t;
    clock_gettime(id, &t);
    long long value = (t.tv_sec * _nanos) + t.tv_nsec;
    return value;
}

/**
 * @brief get current thread's cpu time in nanoseconds
 * @return 
 */
static uint64_t get_cputime()
{
    return gettimex(CLOCK_THREAD_CPUTIME_ID);
}

static uint64_t get_time()
{
    return gettimex(CLOCK_MONOTONIC);
}



// static inline void xsleep0(unsigned int wait_msec)
// {
//     if (wait_msec <= 0)
//     {
//         thrd_yield();
//     }
//     else
//     {
//         struct timespec wait;
//         wait.tv_sec = wait_msec / 1000;
//         wait.tv_nsec = (wait_msec % 1000) * 1000000;
//         thrd_sleep(&wait, NULL);
//     }
// }

static inline void pause(const int count)
{
    // #pragma GCC unroll 10
    for (int ndx = 0; ndx < count; ndx++)
    {
        //__builtin_ia32_pause();
        get_time();
    }

}

/**
 * @brief sleep
 * @param wait_msec > 0 sleep in msecs, 0 yield, < 0 # pause instructions
 */
static inline void xsleep(int wait_msec)
{
    if (wait_msec == 0)
    {
        std::this_thread::yield();
    }
    else if (wait_msec < 0)
    {
        pause(-wait_msec);
    }
    else
    {
        std::chrono::milliseconds wait_ms(wait_msec);
        std::this_thread::sleep_for(wait_ms);
    }
}

/*==*/