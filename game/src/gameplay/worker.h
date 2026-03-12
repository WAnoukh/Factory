#ifndef WORKER_H
#define WORKER_H

#include "cglm/types.h"
#include "gameplay/workpool.h"

#define WORKER_SPEED 3

struct Worker
{
    vec2    pos;
    wid_t   work; 
    int     is_holding_box; 
};

void worker_defaults(struct Worker *worker);

#endif // WORKER_H
