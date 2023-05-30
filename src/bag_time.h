#ifndef BAG_TIME_H
#define BAG_TIME_H

#include <stdint.h>

#ifdef _WIN32
    #include "windows/time_win32.h"
#else
    #include "linux/time_linux.h"
#endif

void bagT_init(void);

int64_t bagT_getTime(void);
int64_t bagT_getFreq(void);
int64_t bagT_toNano(int64_t time);

#endif // BAG_TIME_H
