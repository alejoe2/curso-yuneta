#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> // clock_t, clock(), CLOCKS_PER_SEC

int main(int argc, char *argv[])
{
    clock_t start_t, end_t;
    double total_t;
    int maxLoop = 100;

    start_t = clock();

    if(argc == 2) {
        maxLoop = atoi(argv[1]);
    }

    for(int i = 1; i <= maxLoop; ++i) {
        printf("Hello World: %d \n", i);
    }

    end_t = clock();
    total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
    printf("Total time taken by CPU: %f\n", total_t);

    return 0;
}