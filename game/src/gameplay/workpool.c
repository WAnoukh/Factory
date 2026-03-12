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

Error workpool_own_first_in_zone(WorkPool *pool, Zone *zone, wid_t *out_work_id)
{
    assert(pool);
    assert(zone);
    if(pool->head == WORK_INVALID)
    {
       return ERR_CONTAINER_EMPTY; 
    }

    wid_t wid = pool->head;
    wid_t prec = WORK_INVALID;
    WorkNode *cur_node = pool->nodes + wid;
    int *work_pos = cur_node->work.position;

    int minx = zone->pos[0] * ZONE_SIZE;
    int maxx = (zone->pos[0] + 1) * ZONE_SIZE; 
    int miny = zone->pos[1] * ZONE_SIZE;
    int maxy = (zone->pos[1] + 1) * ZONE_SIZE; 

    int inx = minx <= work_pos[0] && work_pos[0] < maxx;
    int iny = miny <= work_pos[1] && work_pos[1] < maxy;

    while( !(inx && iny) )
    {
        if(wid == WORK_INVALID)
        {
            return ERR_CONTAINER_EMPTY;
        }
        prec = wid;
        wid = cur_node->next;
        cur_node = pool->nodes + wid;
        work_pos = cur_node->work.position;
        inx = minx <= work_pos[0] && work_pos[0] < maxx;
        iny = miny <= work_pos[1] && work_pos[1] < maxy;
    }


    if(prec == WORK_INVALID)
    {
        pool->head = cur_node->next;
    }
    else
    {
        pool->nodes[prec].next = cur_node->next;
    }

    if(wid == pool->tail)
    {
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

Error workpool_remove(WorkPool *pool, wid_t id)
{
    assert(pool);
    assert(pool->tail != WORK_INVALID);
    assert(pool->head != WORK_INVALID);
    assert(pool->count > 0);
    wid_t cur = pool->head;

    if(pool->head == id)
    {
        pool->head = pool->nodes[id].next;
        if(pool->tail == id) { 
            pool->tail = WORK_INVALID; 
        }
        goto end;
    }

    while(cur != WORK_INVALID)
    {
        WorkNode *cur_node = pool->nodes + cur;
        wid_t next = cur_node->next; 
        if(next == id)
        {
            if(pool->tail == id) { pool->tail = cur; }
            cur_node->next = pool->nodes[id].next;
            goto end;
        }
        cur = next;
    }

    return ERR_ELMT_NOT_FOUND;
end:
    pool->nodes[id].next = WORK_INVALID;
    return workpool_remove_owned(pool, id);
}

Work *workpool_get(WorkPool *pool, wid_t work_id)
{
    assert(pool);
    return &pool->nodes[work_id].work;
}

wid_t workpool_get_at(WorkPool *pool, int x, int y)
{
    assert(pool);
    wid_t cur = pool->head;

    while(cur != WORK_INVALID)
    {
        Work *work = &pool->nodes[cur].work;
        if(work->position[0] == x && work->position[1] == y)
        {
            break;
        }
        cur = pool->nodes[cur].next;
    }
    return cur;
}
