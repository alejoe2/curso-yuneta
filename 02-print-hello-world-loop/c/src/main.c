#include <argp.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/***************************************************************************
 *      Structures
 ***************************************************************************/

/*
 *  Used by main to communicate with parse_opt.
 */
#define MIN_ARGS 0
#define MAX_ARGS 0
struct arguments
{
    char *args[MAX_ARGS + 1]; /* positional args */
    int repeat;
};

/***************************************************************************
 *              Prototypes
 ***************************************************************************/
static error_t parse_opt(int key, char *arg, struct argp_state *state);

/* Program documentation. */
static char doc[] = "Command Line Interface Hello World";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/*
 *  The options we understand.
 *  See https://www.gnu.org/software/libc/manual/html_node/Argp-Option-Vectors.html
 */
static struct argp_option options[] = {
    {"repeat", 'r', "TIMES", 0, "Repeat execution 'Hello World' times. Default Times=100", 1},
    {0},
};

/* Our argp parser. */
static struct argp argp = {
    options,
    parse_opt,
    args_doc,
    doc,
};

/***************************************************************************
 *  Parse a single option
 ***************************************************************************/
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    /*
     *  Get the input argument from argp_parse,
     *  which we know is a pointer to our arguments structure.
     */
    struct arguments *arguments = state->input;

    switch(key) {
    case 'r':
        if(arg) {
            arguments->repeat = atoi(arg);
        }
        break;

    case ARGP_KEY_ARG:
        if(state->arg_num >= MAX_ARGS) {
            /* Too many arguments. */
            argp_usage(state);
        }
        arguments->args[state->arg_num] = arg;
        break;

    case ARGP_KEY_END:
        if(state->arg_num < MIN_ARGS) {
            /* Not enough arguments. */
            argp_usage(state);
        }
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

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
 *                    Showhelloloop
 ***************************************************************************/
static inline void showhelloloop(int maxLoop)
{
    struct timespec start_t, end_t, diff_t;

    clock_gettime(CLOCK_MONOTONIC, &start_t);

    for(int i = 1; i <= maxLoop; ++i) {
        printf("Hello World: %d \n", i);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_t);

    diff_t = ts_diff(start_t, end_t);
    printf("Total time taken by CPU: %lu.%lus, Total loops: %d\n", diff_t.tv_sec, diff_t.tv_nsec, maxLoop);
}

/***************************************************************************
 *                      Main
 ***************************************************************************/
int main(int argc, char *argv[])
{
    struct arguments arguments;
    /*
     *  Default values
     */
    memset(&arguments, 0, sizeof(arguments));
    arguments.repeat = 100;
    /*
     *  Parse arguments
     */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    showhelloloop(arguments.repeat);

    return 0;
}
