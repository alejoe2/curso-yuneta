#include <stdio.h>
#include <stdlib.h>
#include <time.h> // clock_t, clock(), CLOCKS_PER_SEC

/***************************************************************************
 *                      Calcule diff time
 ***************************************************************************/
static inline struct timespec ts_diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

/***************************************************************************
 *                      Main
 ***************************************************************************/
int main(int argc, char *argv[])
{
    struct timespec start_t, end_t, diff_t;
    int maxLoop = 100;

    clock_gettime(CLOCK_MONOTONIC, &start_t);

    if(argc == 2) {
        maxLoop = atoi(argv[1]);
    }

    for(int i = 1; i <= maxLoop; ++i) {
        printf("Hello World: %d \n", i);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_t);

    diff_t = ts_diff(start_t, end_t);
    printf("Total time taken by CPU: %lu.%lus\n", diff_t.tv_sec, diff_t.tv_nsec);
    return 0;
}