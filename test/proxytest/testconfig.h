/*
   Copyright 2024 Joseph W. Seigh
   
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef TESTCONFIG_H
#define TESTCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>


// #define STATE_LIVE    0xfeefeffe01234567
// #define STATE_STALE   0xfeefeffe89abcdef
// #define STATE_INVALID 0xfeefeffe76543210


typedef enum {
    smr,            // smrproxy
    arc,            // arcproxy
    rwlock,         // sharedproxy
    mutex,          // mutexproxy
    unsafe,         // noopproxy
    all,            // all w/ summaries only
    all2,           // unsafe, smr, smrlite w/ summaries only
} test_type;



typedef struct {
    test_type test;
    char * name;
    char * desc;
} testcase_t;


typedef struct {
    unsigned int nreaders;      // number of reader threads
    unsigned long count;        // reader loop count
    unsigned int mod;           // reader sleep every mod interations, 0 = no sleep/yield
    int rsleep_ms;              // reader sleep in milliseconds, 0 = thrd_yield, < 0 # pause instructions

    unsigned int wsleep_ms;     // writer update interval

    unsigned int reclaim_ms;    // reclaim thread poll interval

    unsigned int arc_size;      // size of arcproxy 

    testcase_t* test;

    bool verbose;               // more output

    bool quiet;                 // less output

    bool no_stats;              // no data state statistics

} test_config_t;

static const test_config_t test_config_init = { reclaim_ms : 50 , arc_size : 512 };


extern bool getconfig(test_config_t *config, int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif // TESTCONFIG_H