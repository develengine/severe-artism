#ifndef TIME_WIN32_H
#define TIME_WIN32_H

#include <stdio.h>
#include <windows.h>

static inline int64_t bagT_getTime(void)
{
    LARGE_INTEGER ticks;

    if (!QueryPerformanceCounter(&ticks)) {
        fprintf(stderr, "QueryPerformanceCounter(): (%d)\n", GetLastError());
        exit(666);
    }

    return ticks.QuadPart;
}

#endif // TIME_WIN32_H
