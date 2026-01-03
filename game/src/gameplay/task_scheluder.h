#ifndef TASK_SCHELUDER_H
#define TASK_SCHELUDER_H

#include "cglm/types.h"
#include "error/error.h"
#include <stddef.h>

typedef size_t wid_t;
#define WORK_POOL_CAPACITY 512
#define WORK_INVALID ((wid_t)-1)

typedef struct Work 
{
    ivec2   position;
    int     tile;
    float   progression;
}Work;


typedef struct WorkNode
{
    Work  work;
    wid_t next;
}WorkNode;

typedef struct WorkPool
{
    WorkNode nodes[WORK_POOL_CAPACITY];
    wid_t free;
    wid_t count;

    wid_t tail;
    wid_t head;
}WorkPool;

Work work_create(ivec2 pos, int tile);

void workpool_defaults(WorkPool *pool);

Error workpool_add_task(WorkPool *pool, Work work);

Error workpool_own_first(WorkPool *pool, wid_t *out_work_id);

Error workpool_remove_owned(WorkPool *pool, wid_t owned_id);

Work *workpool_get(WorkPool *pool, wid_t work_id);

#endif // TASK_SCHELUDER_H
