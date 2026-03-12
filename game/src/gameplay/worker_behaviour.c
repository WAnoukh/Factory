#include "worker_behaviour.h"
#include "cglm/vec2.h"
#include "game.h"
#include "game_runtime.h"
#include "gameplay/worker.h"
#include <assert.h>

enum BT_Exec_State is_zone_need_boxes(BT_BlackBoard *bb)
{
    struct Game *game = bb->game;

    struct Level *level = game->level;
    for(int i = 0; i < level->zones_count; ++i)
    {
        Zone *zone = level->zones + i;
        if(zone->pending_build > zone->box_count + zone->incoming_box_count)
        {
            zone->incoming_box_count++;
            bb->assigned_zone = i;
            return BT_SUCCESS;
}
    }
    return BT_FAILURE;
}

int get_box_at(struct Game *game, ivec2 pos)
{
    for(int i = 0; i < game->box_count; ++i)
    {
        int *box_pos = game->box[i];
        if(glm_ivec2_eqv(pos, box_pos))
        {
            glm_ivec2_copy(game->box[game->box_count-1], game->box[i]);
            game->box_count--;
            return 1;
        }
    }
    return 0;
}

enum BT_Exec_State go_get_box(BT_BlackBoard *bb)
{
    struct Game *game = bb->game;

    struct Worker *worker = game->workers + bb->worker_index;

    if(!worker->is_holding_box)
    {
        float dist = glm_vec2_norm(worker->pos);
        if(dist > 1) 
        {
            vec2 speed;
            glm_vec2_negate_to(worker->pos, speed);
            glm_vec2_scale_as(speed, WORKER_SPEED * bb->frame->dt, speed);
            glm_vec2_add(speed, worker->pos, worker->pos);
            return BT_RUNNING;
        }
        else
        {
            if(get_box_at(game, GLM_IVEC2_ZERO))
            {
                worker->is_holding_box = 1;
                return BT_SUCCESS;
            }
            else return BT_RUNNING;
        }
    }
    return BT_SUCCESS;
}

enum BT_Exec_State goto_assigned_zone(BT_BlackBoard *bb)
{
    struct Game *game = bb->game;

    struct Worker *worker = game->workers + bb->worker_index;

    Zone *zone = game->level->zones + bb->assigned_zone;

    vec2 target = {zone->pos[0] * ZONE_SIZE, zone->pos[1] * ZONE_SIZE};

    float dist = glm_vec2_distance(target, worker->pos);
    if(dist < 1)
    {
        return BT_SUCCESS;
    }
    else
    {
        vec2 speed;
        glm_vec2_sub(target, worker->pos, speed);
        glm_vec2_scale_as(speed, WORKER_SPEED * bb->frame->dt, speed);
        glm_vec2_add(speed, worker->pos, worker->pos);
        return BT_RUNNING;
    }
}

enum BT_Exec_State drop_box(BT_BlackBoard *bb)
{
    struct Game *game = bb->game;

    struct Worker *worker = game->workers + bb->worker_index;

    Zone *zone = game->level->zones + bb->assigned_zone;

    zone->incoming_box_count--;
    zone->box_count++;
    worker->is_holding_box = 0;

    return BT_SUCCESS;
}

enum BT_Exec_State is_zone_need_worker(BT_BlackBoard *bb)
{
    struct Game *game = bb->game;
    
    struct Level *level = game->level;

    if(bb->is_working)
    {
        return BT_SUCCESS;
    }

    for(int i = 0; i < level->zones_count; ++i)
    {
        Zone *zone = level->zones + i;
        if(zone->pending_build > 0 && zone->workers_count < 1)
        {
            zone->workers_count++;
            bb->assigned_zone = i;
            bb->is_working = 1;
            return BT_SUCCESS;
        }
    }
    return BT_FAILURE;
}

enum BT_Exec_State is_working(BT_BlackBoard *bb)
{
    return bb->is_working ? BT_SUCCESS : BT_FAILURE;
}

enum BT_Exec_State take_job_in_zone(BT_BlackBoard *bb)
{
    struct Game *game = bb->game;

    struct Worker *worker = game->workers + bb->worker_index;

    Zone *zone = game->level->zones + bb->assigned_zone;

    if(zone->pending_build <= 0 || zone->incoming_box_count + zone->box_count <= 0)
    {
        bb->is_working = 0;
        zone->workers_count--;
        return BT_FAILURE;
    }

    if(zone->box_count <= 0)
    {
        return BT_RUNNING;
    }

    Error result = workpool_own_first_in_zone(&game->workpool, zone, &bb->work);
    zone->pending_build--;
    zone->box_count--;
    worker->is_holding_box = 1;
    bb->progress = 0;

    if(result != ERR_OK)
    {
        assert(0);
    }

    return result == ERR_OK ? BT_SUCCESS : BT_FAILURE;
}

enum BT_Exec_State goto_work(BT_BlackBoard *bb)
{
    struct Game *game = bb->game;

    struct Worker *worker = game->workers + bb->worker_index;

    Work *work = workpool_get(&game->workpool, bb->work);

    vec2 work_pos = {(float)work->position[0], (float)work->position[1]};
    float dist = glm_vec2_distance(work_pos, worker->pos);
    if(dist < 0.2f)
    {
        worker->is_holding_box = 0;
        return BT_SUCCESS;
    }
    else{
        vec2 speed;
        glm_vec2_sub(work_pos, worker->pos, speed);
        glm_vec2_scale_as(speed, WORKER_SPEED * bb->frame->dt, speed);
        glm_vec2_add(speed, worker->pos, worker->pos);
        return BT_RUNNING;
    }
}

uint32_t rand_xorshift(uint32_t *state) {
    uint32_t x = *state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    *state = x;

    return x;
}

float rand_float(uint32_t *state) {
    return (float)rand_xorshift(state) / (float)UINT32_MAX;
}

uint32_t seed = 123456;

enum BT_Exec_State wander(BT_BlackBoard *bb)
{
    struct Game *game = bb->game;

    struct Worker *worker = game->workers + bb->worker_index;

    if(bb->wander_remaining <= 0) 
    {
        bb->wander_remaining = rand_float(&seed) * 3;
        bb->wander_dir[0] = (rand_float(&seed) - 0.5f) *2;
        bb->wander_dir[1] = (rand_float(&seed) - 0.5f) *2;
        glm_vec2_normalize(bb->wander_dir);
    }
    else
    {
        vec2 speed;
        glm_vec2_scale_as(bb->wander_dir, WORKER_SPEED / 3 * bb->frame->dt, speed);
        glm_vec2_add(speed, worker->pos, worker->pos);
        bb->wander_remaining -= bb->frame->dt;
    }
    return BT_SUCCESS;
}

enum BT_Exec_State work(BT_BlackBoard *bb)
{
    struct Game *game = bb->game;

    struct Worker *worker = game->workers + bb->worker_index;

    Work *work = workpool_get(&game->workpool, bb->work);

    Zone *zone = game->level->zones + bb->assigned_zone;


    if(bb->progress >= 1)
    {
        workpool_remove_owned(&game->workpool, bb->work);
        tilemap_set_tile(&game->level->tilemap, work->tile, work->position[0], work->position[1]);
        return BT_SUCCESS;
    }
    else{
        bb->progress += 0.4f * bb->frame->dt;
        return BT_RUNNING;
    }
    
}

void create_tree(BT_Tree *tree)
{
    *tree = bt_init();

    BT_Index fallback_indices[] = {1, 7, 12};
    bt_add_fallback_node(tree, fallback_indices, 3);


    BT_Index work_indices[] = {2, 3, 4, 5, 6};
    BT_add_sequ_node(tree, work_indices, 5);

    BT_add_exec_node(tree, is_zone_need_worker);
    BT_add_exec_node(tree, goto_assigned_zone);
    BT_add_exec_node(tree, take_job_in_zone);
    BT_add_exec_node(tree, goto_work);
    BT_add_exec_node(tree, work);

    BT_Index move_indices[] = {8, 9, 10, 11};
    BT_add_sequ_node(tree, move_indices, 4);

    BT_add_exec_node(tree, is_zone_need_boxes);
    BT_add_exec_node(tree, go_get_box);
    BT_add_exec_node(tree, goto_assigned_zone);
    BT_add_exec_node(tree, drop_box);


    BT_add_exec_node(tree, wander);
}
