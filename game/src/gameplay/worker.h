#ifndef WORKER_H
#define WORKER_H

#include "cglm/types.h"
#include "gameplay/workpool.h"

#define WORKER_SPEED 3

enum WorkerState
{
    WS_IDLE,
    WS_REACHING,
    WS_WORKING
};

struct Worker
{
    enum WorkerState state;
    vec2   pos;
    wid_t  work; 
};

void worker_defaults(struct Worker *worker);

#endif // WORKER_H
