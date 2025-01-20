#include <stdint.h>
#include <stdio.h>

// #define EPOCH_NO_WRAP
#include <epoch.h>
// using epoch_t = uint64_t;

#include <atomic>

    constexpr uint64_t max63 = (uint64_t) INT64_MAX;
    constexpr uint64_t max31 = (uint64_t) INT32_MAX;



void print(epoch_t x0, epoch_t x1) {
    fprintf(stdout, "  x0 < x1  %d\n", x0 < x1);
    fprintf(stdout, "  x0 > x1  %d\n", x0 > x1);
    fprintf(stdout, "  x0 == x1  %d\n", x0 == x1);
    fprintf(stdout, "  x0 != x1  %d\n", x0 != x1);
   
    fprintf(stdout, "\n");
}

enum relop {
    lt,
    le,
    eq,
    ge,
    gt,
    ne,
};
static std::string names[] = {"<", "<=", "==", ">=", ">", "!="};

static bool cmp(epoch_t a, relop op, epoch_t b)
{
    switch(op) {
    case lt: return (a < b);
    case le: return (a <= b);
    case eq: return (a == b);
    case ge: return (a >= b);
    case gt: return (a > b);
    case ne: return (a != b);
    }
    return false;
}


static bool test(uint64_t a, uint64_t b, relop op, bool expected)
{
    epoch_t x0 = a;     
    epoch_t x1 = b;
    bool actual = cmp(x0, op, x1);
    bool result = actual == expected; 

    fprintf(stdout, "%016llx %s %016llx => %d,  expected=%d\n", x0, names[op].c_str(), x1, actual, expected);
    if (!result)
        print(x0, x1);
    return result;
}


int main()
{
    fprintf(stdout, "max63 = %016llx\n", max63);
    fprintf(stdout, "max31 = %016llx\n", max31);
    fprintf(stdout, "epoch_t size = %d\n", sizeof(epoch_t));
    fprintf(stdout, "std::atomic<epoch_t>::is_always_lock_free=%d\n", std::atomic<epoch_t>::is_always_lock_free);
    fprintf(stdout, "\n");

    test(max63 - 31, max63 + 31, lt, true);
    test(max31 - 3, max31 + 1, lt, true);
    test(max31 + 1, 0, lt, false);

#ifndef EPOCH_NO_WRAP
    test(-16, 16, lt, true);
#else
    test(-16, 16, lt, false);
#endif

    test(0, 0, eq, true);
    test(1, 1, ne, false);

    epoch_t x0 = 20;
    epoch_t x1 = x0;
    x1 += 2;

    test(x0, x1, lt, true);

    return 0;
}