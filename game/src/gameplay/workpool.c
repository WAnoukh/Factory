#include "workpool.h"
#include "error/error.h"
#include <assert.h>

Work work_create(ivec2 pos, int tile)
{
    return (Work)
    {
        .position =     {pos[0], pos[1]},
        .tile =         tile,
        .progression =  0,
    };
}

void workpool_defaults(WorkPool *pool)
{
    assert(pool);
    pool->count = 0;
    pool->free = 0;

    pool->head = WORK_INVALID;
    pool->tail = WORK_INVALID;

    for(int i = 0; i < WORK_POOL_CAPACITY - 1; ++i)
    {
        pool->nodes[i].next = i + 1;
    }
    pool->nodes[WORK_POOL_CAPACITY - 1].next = WORK_INVALID;
}

Error workpool_add_task(WorkPool *pool, Work work)
{
    assert(pool);
    if(pool->count == WORK_POOL_CAPACITY)
    {
        return ERR_CONTAINER_FULL;
    }
    wid_t wid = pool->free;
    WorkNode *free_node = pool->nodes + wid;
    pool->free = free_node->next;
    *free_node = (WorkNode)
    {
        .work = work,
        .next = WORK_INVALID,
    };
    pool->count ++;

    if(pool->tail != WORK_INVALID)
    {
        pool->nodes[pool->tail].next = wid;
    }
    else
    {
        pool->head = wid;
    }

    pool->tail = wid;

    return ERR_OK;
}

Error workpool_own_first(WorkPool *pool, wid_t *out_work_id)
{
    assert(pool);
    if(pool->head == WORK_INVALID)
    {
       return ERR_CONTAINER_EMPTY; 
    }

    wid_t wid = pool->head;
    WorkNode *cur_node = pool->nodes + wid;

    if(cur_node->next != WORK_INVALID)
    {
        pool->head = cur_node->next;
    }
    else
    {
        assert(wid == pool->tail);
        pool->head = WORK_INVALID;
        pool->tail = WORK_INVALID;
    }

    cur_node->next = WORK_INVALID;
    *out_work_id = wid;
    return ERR_OK;
}

Error workpool_remove_owned(WorkPool *pool, wid_t owned_id)
{
    assert(owned_id != WORK_INVALID);
    assert(pool);
    if(pool->count == 0) { return ERR_CONTAINER_EMPTY; }

    WorkNode *node = pool->nodes + owned_id;
    assert(node->next == WORK_INVALID);
    node->next = pool->free;
    pool->free = owned_id;
    --pool->count;

    return ERR_OK;
}

Work *workpool_get(WorkPool *pool, wid_t work_id)
{
    return &pool->nodes[work_id].work;
}

