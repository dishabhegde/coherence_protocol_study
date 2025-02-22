#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Every component needs to define the following:
int tick();
int finish(int);
int destroy(void);

// Every componet also needs to define an init that returns
//   a pointer specific to that type of component

typedef struct _sim_interface {
    int (*tick)(void);
    int (*finish)(int);
    int (*destroy)(void);
} sim_interface;

// Flag set by engine if verbose flag is passed in,
// components can read flag
extern int CADSS_VERBOSE;
extern int processorCount;

typedef struct {
    int busrd;
    int buswr;
    int busupd;
    int shared;
    int data;
} bus_req_stats_t;

typedef struct{
    int mem_transfers;
    int bus_reqs;
    bus_req_stats_t per_req_stats;
    int *cumulative_wait_time;
}stats_t;

#endif
