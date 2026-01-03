#include "worker.h"

void worker_defaults(struct Worker *worker)
{
    *worker = (struct Worker)
    {
        .pos = {0, 0},
        .work = WORK_INVALID,
        .state = WS_IDLE,
    };
}

