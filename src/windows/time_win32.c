#include "bag_time.h"

struct
{
    int64_t mul;
    int64_t freq;
} bagT;


void bagT_init(void)
{
    LARGE_INTEGER freq;

    if (!QueryPerformanceFrequency(&freq)) {
        fprintf(stderr, "QueryPerformanceFrequency(): (%d)\n", GetLastError());
        exit(666);
    }

    bagT.mul = (1000 * 1000 * 1000) / freq.QuadPart;
    bagT.freq = freq.QuadPart;
}


int64_t bagT_toNano(int64_t time)
{
    return time * bagT.mul;
}


int64_t bagT_getFreq(void)
{
    return bagT.freq;
}
