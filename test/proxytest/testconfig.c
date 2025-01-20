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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h>

#include "testconfig.h"

enum opts
{
    reclaim_opt = 256,
    no_stats_opt,
};

static testcase_t tests[] = {
    { smr, "smr", "smrproxy" },
    { arc, "arc", "arcproxy" },
    { rwlock, "rwlock", "rwlock based proxy" },
    { mutex, "mutex", "mutex based proxy" },
    { unsafe, "unsafe", "unsafe access" },
    { all, "all", "all tests w/ summaries only"},
    { all2, "all2", "unsafe, smr, smrlite w/ summaries only"},
    { 0, NULL, NULL }
};

static testcase_t* find_test(char *opt)
{
    for (int ndx = 0; tests[ndx].name != NULL; ndx++) {
        if (strcmp(opt, tests[ndx].name) == 0)
            return &tests[ndx];
    }
    fprintf(stderr, "Unknown reader type %s\n", opt);
    return &tests[0];
}

bool getconfig(test_config_t *config, int argc, char **argv) {

    static struct option long_options[] = {
        {"count", required_argument, 0, 'c'},
        {"nreaders", required_argument, 0, 'n'},
        {"mod", required_argument, 0, 'm'},
        {"rsleep_ms", required_argument, 0, 'r'},
        {"wsleep_ms", required_argument, 0, 'w'},
        {"reclaim_ms", required_argument, 0, reclaim_opt},
        {"size", required_argument, 0, 's'},
        {"type", required_argument, 0, 't'},
        {"no_stats", no_argument, 0, no_stats_opt},
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {0, 0, 0, 0}
    };

    *config = test_config_init;

    bool verbose = false;
    bool help = false;

    while (1)
    {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        int c = getopt_long(argc, argv, "c:n:m:r:s:w:t:vqh", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
            case 'c':
                config->count = atol(optarg);
                break;
            case 'n':
                config->nreaders = atoi(optarg);
                break;
            case 'm':
                config->mod = atoi(optarg);
                break;
            case 'r':
                config->rsleep_ms = atoi(optarg);
                break;
            case 'w':
                config->wsleep_ms = atoi(optarg);
                break;
            case reclaim_opt:
                config->reclaim_ms = atoi(optarg);
                break;
            case no_stats_opt:
                config->no_stats = true;
                break;
            case 's':
                config->arc_size = atoi(optarg);
                break;
            case 't':
                config->test = find_test(optarg);
                break;
            case 'v':
                config->verbose = true;
                verbose = true;
                break;
            case 'q':
                config->quiet = true;
                break;
            case 'h':
                help = true;
                break;
            case '?':
                help = true;
                break;
            default:
                break;
        }

    }


    if (config->mod <= 0)
    {
        config->mod = config->count;
    }

    if (config->test == NULL)
        config->test = tests;


    if (help)
    {
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -c --count <arg>  reader access count (default %u)\n", test_config_init.count);
        fprintf(stderr, "  -n --nreaders <arg>  number of reader threads (default %u)\n", test_config_init.nreaders);
        fprintf(stderr, "  -m --mod <arg>  reader sleep on every n'th access (default %u)\n", test_config_init.mod);
        fprintf(stderr, "  -r --rsleep_ms <arg>  reader sleep duration in milliseconds (default %d)\n", test_config_init.rsleep_ms);
        fprintf(stderr, "  -w --wsleep_ms <arg>  writer sleep duration in milliseconds (default %u)\n", test_config_init.wsleep_ms);
        fprintf(stderr, "     --reclaim_ms <arg> reclaim poll interval in milliseconds (default %u)\n", test_config_init.reclaim_ms);
        fprintf(stderr, "     --no_stats no data state statistics (default false)\n");
        fprintf(stderr, "  -s --size <arg> size of arcproxy (default %u)\n", test_config_init.arc_size);
        fprintf(stderr, "  -t --type testcase:\n");
        for (int ndx = 0; tests[ndx].name != NULL; ndx++)
        {
            fprintf(stderr, "    %s -- %s\n", tests[ndx].name, tests[ndx].desc);
        }

        fprintf(stderr, "  -v --verbose show config values (default false)\n");
        fprintf(stderr, "  -q --quiet less output (default false)\n");
        return false;
    }

    if (verbose)
    {
        fprintf(stderr, "Test configuration:\n");
        fprintf(stderr, "  count=%u\n", config->count);
        fprintf(stderr, "  nreaders=%u\n", config->nreaders);
        fprintf(stderr, "  mod=%u\n", config->mod);
        fprintf(stderr, "  rsleep_ms=%d\n", config->rsleep_ms);
        fprintf(stderr, "  wsleep_ms=%u\n", config->wsleep_ms);
        fprintf(stderr, "  reclaim_ms=%u\n", config->reclaim_ms);
        fprintf(stderr, "  no_stats=%s\n", config->no_stats ? "true" : "false");
        fprintf(stderr, "  size=%u\n", config->arc_size);
        fprintf(stderr, "  type=%s\n", config->test->name);
        fprintf(stderr, "  quiet=%s\n", config->quiet ? "true" : "false");
        fprintf(stderr, "  test=%s\n", config->test->name);
        fprintf(stderr, "\n");
// #ifndef SMRPROXY_MB
//         fprintf(stderr, "  SMRPROXY_MB not defined\n");
// #else
//         fprintf(stderr, "  SMRPROXY_MB defined\n");
// #endif
    }

    return true;
}