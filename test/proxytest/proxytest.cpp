#include <mutex>
#include <shared_mutex>
#include <thread>


#include <smrproxy.h>
#include <sharedproxy.h>
#include <arcproxy.h>

#include "proxytest.h"
#include "testconfig.h"

#include <stdio.h>

static void execute(summary_t& summary, test_config_t& config, test_type test)
{
    std::mutex m;
    std::shared_mutex shared_mutex;

    Stats* stats = new Stats();

    switch (test)
    {
        case smr: {
            smrproxy* const proxy = new smrproxy();

            exec_test<smr_obj_base, smr_ref, smrproxy, std::mutex>(stats, proxy, &m, config);

            delete proxy;
        }
        break;

        case arc: {
            arcproxy* proxy = new arcproxy(config.arc_size);

            exec_test<arc_obj_base, arc_ref_t, arcproxy, std::mutex>(stats, proxy, &m, config);

            delete proxy;
        }
        break;

        case rwlock: {
            sharedproxy* proxy = new sharedproxy(shared_mutex);

            exec_test<shared_obj_base, std::shared_lock<std::shared_mutex>, sharedproxy, std::shared_mutex>(stats, proxy, &shared_mutex, config);
            
            delete proxy;
        }
        break;

        case mutex: {
            mutexproxy* proxy = new mutexproxy(&m);

            exec_test<shared_obj_base, std::mutex, mutexproxy, std::mutex>(stats, proxy, &m, config);

            delete proxy;
        }
        break;

        case unsafe: {
            noopproxy* proxy = new noopproxy();

            exec_test<shared_obj_base, noopproxy, noopproxy, std::mutex>(stats, proxy, &m, config);

            delete proxy;
        }
        break;

        case all:
        break;

        default:
        break;
    }

    stats->printStats(summary, config);

    delete stats;
}

int main(int argc, char **argv)
{
    test_config_t config = {};

    if (!getconfig(&config, argc, argv))
        return 1;

    summary_t unsafe_summary, summary;

    test_type test = config.test->test;
    switch (test)
    {
        case all:
        case all2:
            summary_t::print_header();

            execute(unsafe_summary, config, unsafe);      // calculate unsyncrhonized base cpu/elapsed times
            summary_t::print_summary(unsafe_summary, unsafe_summary, "unsafe");

            std::this_thread::yield();

            summary = {};
            execute(summary ,config, smr);
            summary_t::print_summary(unsafe_summary, summary, "smr");

            if (test == all2)
                break;

            std::this_thread::yield();

            summary = {};
            execute(summary ,config, arc);
            summary_t::print_summary(unsafe_summary, summary, "arc");

            execute(summary ,config, rwlock);
            summary_t::print_summary(unsafe_summary, summary, "rwlock");

            execute(summary ,config, mutex);
            summary_t::print_summary(unsafe_summary, summary, "mutex");

            break;

        default:
            if (!config.quiet)
                fprintf(stdout, "testcase: %s\n", config.test->name);

            execute(summary ,config, test);
            break;
    }

    return 0;
}