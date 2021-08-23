#ifndef _TIMER_H
#define TIMER_H = 1
#include <time.h>
#define beginClock(testname) {char * t_name = testname; clock_t begin = clock();
#define endClock clock_t end = clock(); double time_spent_ms = (double)(end - begin)*1000/CLOCKS_PER_SEC; printf("TEST %s Time Taken: %f ms\n", t_name, time_spent_ms);}
#endif