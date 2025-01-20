#pragma once

#include <type_traits>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <latch>
#include <string>

#include <proxy.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/resource.h>
#include <locale.h>
#include <string.h>

#include "testconfig.h"
#include "testutil.h"

// #include <stdatomic.h>


struct summary_t
{
    double  avg_cputime = 0.0;      // average cpu time per locked read operation
    double  avg_elapsed = 0.0;      // average elapsed time per locked read operation
    bool invalid_state = false;     // invalid or other data states seen
    bool mem_leak = false;          // allocs != deletes
    uint32_t allocs = 0;
    uint32_t deallocs = 0;

    static void print_header()
    {
        fprintf(stderr, "%-8s %-17s %-13s %-17s %-13s\n",
            "test", "    cpu time", "(unsafe diff)", "    elapsed time", "(unsafe diff)");
    }

    static void print_summary(const summary_t& base, const summary_t& test_summary, std::string name)
    {
        double adj_cputime = test_summary.avg_cputime - base.avg_cputime;
        if (adj_cputime < 0)
            adj_cputime = 0;

        double adj_elapsed = test_summary.avg_elapsed - base.avg_elapsed;
        if (adj_elapsed < 0)
            adj_elapsed = 0;

        fprintf(stderr, "%-8s %'11.3f nsecs (%'11.3f) %'11.3f nsecs (%'11.3f)%s%s",
            name.c_str(),
            test_summary.avg_cputime,
            adj_cputime,
            test_summary.avg_elapsed,
            adj_elapsed,
            test_summary.invalid_state ? "*" : "",
            test_summary.mem_leak ? "!" : "");

            if (test_summary.mem_leak)
                fprintf(stderr, " allocs=%u deallocs=%u", test_summary.allocs, test_summary.deallocs);
            fprintf(stderr, "\n");

    }
};

// constexpr uint64_t STATE_LIVE = 0xfeefeffe01234567ull;
// constexpr uint64_t STATE_STALE = 0xfeefeffe89abcdefull;
// constexpr uint64_t STATE_INVALID = 0xfeefeffe76543210ull;

constexpr uint32_t STATE_LIVE = 0x01010101;
constexpr uint32_t STATE_STALE = 0x20202020;
constexpr uint32_t STATE_INVALID = 0xefefefef;

enum data_state
{
    live = STATE_LIVE,
    stale = STATE_STALE,
    invalid = STATE_INVALID,
    other = 0

};

class Stats {
public:

    uint32_t live;          // count of live data objects
    uint32_t stale;         // count of stale data objects pending retire
    uint32_t invalid;       // count of possibly retired data objects
    uint32_t other;         // count of data object none of the above states

    uint32_t alloc_data;    // number of data allocations by writer
    uint32_t delete_data;   // number of data deallocations by writer

    uint32_t retire_count;  // number of proxy retire invocations by writer
    uint64_t retire_time;   // sum of proxyretire times

    uint32_t read_count;    // sum of all reader counts
    uint64_t read_time;     // sum of all reader runtimes

    uint64_t elapsed;       // sum of elapsed non-sleep time for all readers

    // rusage stats
    uint32_t ru_nvcsw;      // voluntary context switches
    uint32_t ru_nivcsw;     // involuntary context switches
    uint64_t ru_utime;      // user cpu time
    uint64_t ru_stime;      // system cpu time

    template<typename T>
    requires std::is_arithmetic_v<T>
    void add(T &counter, T addend)
    {
        std::atomic_ref<T> x(counter);
        x.fetch_add(addend, std::memory_order_relaxed);
    }

    // void add(uint64_t &count, uint64_t addend)
    // {
    //     std::atomic_ref<uint64_t> x(count);
    //     x.fetch_add(addend, std::memory_order_relaxed);
    // }

    void mergestats(Stats &target)
    {
        struct rusage usage;
        getrusage(RUSAGE_THREAD, &usage);
        ru_utime = timeval_nsecs(&usage.ru_utime);
        ru_stime = timeval_nsecs(&usage.ru_stime);
        ru_nvcsw = (uint32_t) usage.ru_nvcsw;
        ru_nivcsw = (uint32_t) usage.ru_nivcsw;

        add(target.live, live);
        add(target.stale, stale);
        add(target.invalid, invalid);
        add(target.other, other);

        add(target.alloc_data, alloc_data);
        add(target.delete_data, delete_data);

        add(target.retire_count, retire_count);
        add(target.retire_time, retire_time);

        add(target.read_count, read_count);
        add(target.read_time, read_time);

        add(target.ru_nvcsw, ru_nvcsw);
        add(target.ru_nivcsw, ru_nivcsw);
        add(target.ru_utime, ru_utime);
        add(target.ru_stime, ru_stime);
    }

    static double avg(double nanoseconds, double count, double scale)
    {
        return count == 0.0 ? 0.0 : (nanoseconds/count)/scale;
    }

    void printStats(summary_t& summary, test_config_t& config)
    {

        char *current = setlocale(LC_NUMERIC, "");
        char current_locale[64];
        strncpy(current_locale, current, 63);

        locale_t templocale = newlocale(LC_NUMERIC_MASK, current_locale, (locale_t) 0);
        locale_t prevlocale = uselocale(templocale);  // or (locale_t) 0 or LC_GLOBAL_LOCALE

        summary.avg_cputime = avg(read_time, read_count, 1);
        summary.avg_elapsed = avg(elapsed, config.count, 1);
        summary.invalid_state = (invalid + other) > 0;
        summary.mem_leak = (alloc_data != delete_data);
        summary.allocs = alloc_data;
        summary.deallocs = delete_data;

        if (config.test->test ==  all || config.test->test ==  all2)
        {
            // no output
        }

        else if (config.quiet)
        {
            fprintf(stderr, "%-8s %'11.3f nsecs  %'11.3f nsecs%s%s",
                config.test->name,
                summary.avg_cputime,
                summary.avg_elapsed,
                summary.invalid_state ? "*" : "",
                summary.mem_leak ? "!" : "");

            if (summary.mem_leak)
                fprintf(stderr, " allocs=%u deallocs=%u", alloc_data, delete_data);

            fprintf(stderr, "\n");
        }

        else
        {
            fprintf(stderr, "Statistics:\n");
            fprintf(stderr, "  reader thread count = %d\n", config.nreaders);
            fprintf(stderr, "  read_count = %'ld\n", read_count);
            fprintf(stderr, "  elapsed cpu read_time = %'lld nsecs\n", read_time);
            fprintf(stderr, "  avg cpu read_time = %'9.3f nsecs\n", summary.avg_cputime);
            fprintf(stderr, "  elapsed read_time = %'lld nsecs\n", elapsed);
            fprintf(stderr, "  avg elapsed read time =  %'9.3f nsecs\n", summary.avg_elapsed);

            if (!config.no_stats)
            {
                fprintf(stderr, "  data state counts:\n", live);
                fprintf(stderr, "    live = %'ld\n", live);
                fprintf(stderr, "    stale = %'ld\n", stale);
                fprintf(stderr, "    invalid = %'ld\n", invalid);
                fprintf(stderr, "    other = %'ld\n", other);
            }

            fprintf(stderr, "  retire_count = %'ld\n", retire_count);
            fprintf(stderr, "  elapsed retire_time = %'lld nsecs\n", retire_time);
            fprintf(stderr, "  avg retire_time = %'9.3f usecs\n", avg(retire_time, retire_count, 1e3));

            fprintf(stderr, "  data allocs = %'ld\n", alloc_data);
            fprintf(stderr, "  data deletes = %'ld\n", delete_data);

            fprintf(stderr, "  voluntary context switches = %'ld\n", ru_nvcsw);
            fprintf(stderr, "  involuntary context switches = %'ld\n", ru_nivcsw);
            fprintf(stderr, "  user cpu time = %'lld nsecs\n", ru_utime);
            fprintf(stderr, "  system cpu time = %'lld nsecs\n", ru_stime);
            fflush(stderr);
        }


        uselocale(prevlocale);
        freelocale(templocale);

        return;
    }

};

struct SharedData
{
    std::atomic<data_state> state = live;

    Stats* stats;

    uint64_t retire_time = 0;
    uint64_t reclaim_time = 0;


    SharedData(Stats* stats) : stats(stats)
    {
        std::atomic_ref(stats->alloc_data).fetch_add(1, std::memory_order_relaxed);
        state.store(live, std::memory_order_relaxed);
    }
    ~SharedData()
    {
        state.store(invalid, std::memory_order_release);
        reclaim_time = get_time();
        uint64_t elapsed = (reclaim_time - retire_time);
        std::atomic_ref(stats->retire_time).fetch_add(elapsed, std::memory_order_relaxed);
        std::atomic_ref(stats->delete_data).fetch_add(1, std::memory_order_relaxed);
    }

    void retire()
    {
        // std::atomic_thread_fence(std::memory_order_release);
        state.store(stale, std::memory_order_release);
        std::atomic_ref(stats->retire_count).fetch_add(1, std::memory_order_relaxed);
        retire_time = get_time();
    }
};


template<typename B, BasicLockable R, ProxyType<R, B> P, BasicLockable M>
class Env
{
    using shared_data_t = managed_obj<B, SharedData>;

private:
    std::atomic<shared_data_t*> pdata = nullptr;

public:
    P* proxy;
    M* mutex;                   // writer mutex
    std::latch* latch;

    std::atomic_bool active{false};       // used by writer to determine if readers active

    test_config_t &config;

    Stats* stats;

    std::atomic<Env*> next;     // always points to self
    
    Env(Stats* stats, P* proxy, M* mutex, std::latch* latch, test_config_t& config)
        : latch(latch), config(config)
    {
        this->proxy = proxy;
        this->mutex = mutex;
        this->stats = stats;
        this->next.store(this, std::memory_order_relaxed);
    }

    bool isActive() { return active.load(std::memory_order_acquire); }
    void setActive(bool value) { active.store(value, std::memory_order_release); active.notify_all(); }

    shared_data_t* swapData(shared_data_t *data) { return pdata.exchange(data, std::memory_order_acq_rel); }
    shared_data_t* getData() { return pdata.load(std::memory_order_acquire); }

};


template<typename B, BasicLockable R, typename P, BasicLockable M>
int testreader(Env<B, R, P, M>* env)
{
    using shared_data_t = managed_obj<B, SharedData>;
    using env_t = Env<B, R, P, M>;

    constexpr unsigned int _loop_unroll = 10;

    env->latch->arrive_and_wait();

    const unsigned int mod = env->config.mod;
    const auto rsleep_ms = env->config.rsleep_ms;  

    const bool update_stats = !env->config.no_stats;

    Stats stats = {};        // local stats

    uint64_t t0 = get_cputime();    //start time

    R* rlock = env->proxy->acquire_ref();      // ****************

    const int rcount = rsleep_ms;


    // _Pragma("GCC unroll 10")
    #pragma GCC unroll _loop_unroll
    for (int ndx = 0; ndx < env->config.count; ndx++)
    {
        std::scoped_lock m(*rlock);

        shared_data_t* _pdata = env->getData();

        if (mod > 0 && (ndx%mod) == 0)
        {
            std::this_thread::yield();
        }

        env_t* _env = env;
        for (int ndx = 0; ndx < rcount; ndx++)
        {
            // __builtin_ia32_pause();
            // get_time();

            _env = _env->next.load(std::memory_order_acquire);
        }


        data_state state = _pdata->state.load(std::memory_order_relaxed);
        if (update_stats) {
            switch (state)
            {
                case live: stats.live++; break;
                case stale: stats.stale ++; break;
                case invalid: stats.invalid++; break;
                default: stats.other++; break;
            }
        }

        // scoped_lock
    }

    uint64_t t1 = get_cputime();    //end time\

    env->proxy->release_ref(rlock);

    stats.read_count = env->config.count;
    stats.read_time = (t1 - t0);

    stats.mergestats(*env->stats);

    return 0;
};

template<typename B, BasicLockable R, typename P, BasicLockable M>
int testwriter(Env<B, R, P, M>* env)
{
    using shared_data_t = managed_obj<B, SharedData>;

    env->latch->arrive_and_wait();

    // Stats stats;        // local stats

    uint64_t t0 = get_cputime();    //end time

    if (env->config.wsleep_ms == 0)
    {
        env->active.wait(true);
    }

    else
    {
        while (env->isActive())
        {
            xsleep(env->config.wsleep_ms);

            {
                std::scoped_lock m(*env->mutex);
                // allocate new object and retire old one
                shared_data_t* pdata = new shared_data_t(env->stats);

                shared_data_t* pdata2 = env->swapData(pdata);
                pdata2->retire();
                std::atomic_thread_fence(std::memory_order_release);    // ?
                env->proxy->retire(pdata2);
            }
        }
    }

    uint64_t t1 = get_cputime();    //end time


    // stats.mergestats(env->stats);

    return 0;
};

template<typename B, BasicLockable R, typename P, BasicLockable M>
void exec_test(Stats* stats, P* proxy, M* mutex, test_config_t& config)
{
    using shared_data_t = managed_obj<B, SharedData>;

    std::latch latch(config.nreaders + 1);  // #readers + 1 writer

    Env<B, R, P, M> env(stats, proxy, mutex, &latch, config);

    env.setActive(true);
    shared_data_t* shared_data = new shared_data_t(env.stats);
    env.swapData(shared_data);

    unsigned int nreaders = config.nreaders;

    std::thread readers[nreaders];
    for (int ndx = 0; ndx < nreaders; ndx++)
    {
        readers[ndx] = std::thread(testreader<B, R, P, M>, &env);
    }

    uint64_t e0 = get_time();

    std::thread writer(testwriter<B, R, P, M>, &env);

    for (int ndx = 0; ndx < nreaders; ndx++)
    {
        readers[ndx].join();
    }

    env.setActive(false);

    writer.join();

    uint64_t e1 = get_time();
    env.stats->elapsed = (e1 - e0);

    shared_data = env.swapData(nullptr);
    shared_data->retire();
    delete shared_data;

    // env.stats.printStats(summary, config);

    // return env.stats;

};

/*==*/