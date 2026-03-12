#include "game.h"
#include "GLFW/glfw3.h"
#include "camera/top_down_camera.h"
#include "cglm/vec2-ext.h"
#include "cglm/vec2.h"
#include "console/log.h"
#include "engine.h"
#include "game_runtime.h"
#include "gameplay/worker.h"
#include "inputs.h"
#include "level/level.h"
#include "memory/arena.h"
#include "rendering/rendering.h"
#include "rendering/texture.h"
#include "transform/transform.h"
#include <assert.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_OPENGL3
#define CIMGUI_USE_GLFW
#include "cimgui.h"
#include "cimgui_impl.h"

#define WORKERS_MAX 10

#define TILE_COST 10

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

void game_start(struct Game *game)
{
    game->time_scale = 1;
    game->camera = camera_get_default();
    rendering_set_camera(&game->camera);
    game->level = level_create_empty(MAP_SIZE, MAP_SIZE, game->arenas->level);
    tilemap_fill(&game->level->tilemap, 1);
    game->level->tilemap.tile[0] = 2;
    tdcamera_defaults(&game->tdcamera);
    tdcamera_manage(&game->tdcamera, &game->camera);
    
    workpool_defaults(&game->workpool);

    game->workers_count = WORKERS_MAX;
    game->workers = arena_allocate_align(game->arenas->main, sizeof(struct Worker) * WORKERS_MAX, alignof(struct Worker));
    game->w_runtimes = arena_allocate_align(game->arenas->main, sizeof(BT_Runtime) * WORKERS_MAX, alignof(BT_Runtime));
    game->w_bb = arena_allocate_align(game->arenas->main, sizeof(BT_BlackBoard) * WORKERS_MAX, alignof(BT_BlackBoard));
    if(!game->workers)
    {
        assert(0); abort();
    }
    for(int i = 0; i < game->workers_count; ++i)
    {
        worker_defaults(game->workers + i);
        BT_BlackBoard *bb = game->w_bb + i;
        bb->worker_index = i;
        bb->game = game;
    }

    game->money = 10000;

    game->box_count = 0;

    BT_Tree *tree = &game->worker_bt;
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

int dragging = 0;
vec2 drag_start_pixel;
vec2 drag_start;
int cur_tile = 1;
int cur_tile_x;
int cur_tile_y;

void pixel_to_world(struct Camera *camera, struct WindowContext *window, const vec2 pixel, vec2 out_world)
{
    camera_screen_to_world(camera, (vec2){
            pixel[0] / (float)window->width, 
            1 - (pixel[1] / (float)window->height)}
            ,out_world);
}

void draw_ui(struct Game *game, struct FrameContext *frame)
{
    assert(game);
    assert(frame);

    struct InputInfo        inputs = frame->inputs;
    struct WindowContext    window = frame->window;

    mat3 char_transform;


    const float char_ratio = 5.f/3;
    const float char_width = 0.01f;
    const float char_height = char_width * char_ratio;

    const float start_offset = 0.03f;
    vec2 draw_pos = {(start_offset+char_width/2), 1-(start_offset+char_height/2)*window.ratio};
    vec2 draw_size = {char_width, char_height*window.ratio};

    compute_transform(char_transform, draw_pos, draw_size);
    unsigned int char_shader = shaders_use_atlas(get_texture_font_atlas(), 5, 3);
    draw_transformed_quad_screen_space(char_shader, char_transform, (float[]){1,1,1}, 1);

    char money_string[16];
    itoa(game->money, money_string, 10);
    char *cur_char = money_string;
    while(*cur_char != '\0')
    {
        draw_pos[0] += char_width * 1.1f;
        int atlas_x, atlas_y;
        int atlas_index = 40 + *cur_char - '0';
        atlas_index_to_coordinates(get_texture_font_atlas(), atlas_index, &atlas_x, &atlas_y);
        unsigned int char_shader = shaders_use_atlas(get_texture_font_atlas(), atlas_x, atlas_y);
        compute_transform(char_transform, draw_pos, draw_size);
        draw_transformed_quad_screen_space(char_shader, char_transform, (float[]){1,1,1}, 1);
        ++cur_char;
    }
}

void game_update(struct Game *game, struct FrameContext *frame)
{
    assert(game);
    assert(game->engine);
    assert(frame);

    struct InputInfo        inputs = frame->inputs;
    struct WindowContext    window = frame->window;
    struct Engine *         engine = game->engine;

    frame->dt = frame->real_dt * game->time_scale;
    frame->time += frame->dt;

    for(int i = 0; i < game->workers_count; i++)
    {
        game->w_bb[i].frame = frame;
    }

    if(is_key_pressed(inputs, GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(engine->window_ctx, 1);
    }

    if(is_key_pressed(inputs, GLFW_KEY_GRAVE_ACCENT))
    {
        engine->show_console = !engine->show_console;
    }

    if(is_key_pressed(inputs, GLFW_KEY_SPACE))
    {
        game->time_scale = game->time_scale ? 0 : 1;
    }

    tdcamera_update(&game->tdcamera, frame);

    camera_compute_view(&game->camera, window.ratio);

    /////////// construction
    for(int i = GLFW_KEY_1; i <= GLFW_KEY_9; ++i)
    {
        if(is_key_pressed(inputs, i))
        {
            cur_tile = i - GLFW_KEY_0;
        }
    }
    atlas_index_to_coordinates(get_atlas_tilemap(), cur_tile-1, &cur_tile_x, &cur_tile_y);

    if(!dragging)
    {
        drag_start_pixel[0] = inputs.mouse_x;
        drag_start_pixel[1] = inputs.mouse_y;
        pixel_to_world(&game->camera, &window, drag_start_pixel, drag_start);
    }

    //create box
    int spawn_point_available= 1;
    for(int i = 0; i < game->box_count; ++i)
    {
        int *pos = game->box[i];
        if(pos[0] == 0 && pos[1] == 0)
        {
            spawn_point_available = 0;
            break;
        }
    }
    if(spawn_point_available)
    {
        assert(game->box_count < BOX_MAX);
        glm_ivec2_copy((ivec2){0, 0}, game->box[game->box_count++]) ;
    }
    
    //calculate drag bounding
    vec2 world_mouse_pos = {inputs.mouse_x, inputs.mouse_y};
    pixel_to_world(&game->camera, &window, world_mouse_pos, world_mouse_pos);

    vec2 bound_min, bound_max;
    glm_vec2_minv(world_mouse_pos, drag_start, bound_min);
    glm_vec2_maxv(world_mouse_pos, drag_start, bound_max);

    glm_vec2_floor(bound_min, bound_min);
    glm_vec2_ceil(bound_max, bound_max);

    if(!dragging && is_mouse_down(inputs, 0))
    {
        dragging = 1; 
    }
    if(dragging && !is_mouse_down(inputs, 0))
    {
        dragging = 0;
        glm_vec2_copy(world_mouse_pos, drag_start);
        
        int x_start = (int)glm_max(bound_min[0], 0);
        int y_start = (int)glm_max(bound_min[1], 0);
        int x_end   = glm_imin((int)bound_max[0], game->level->tilemap.width);
        int y_end   = glm_imin((int)bound_max[1], game->level->tilemap.height);
        for(int x = x_start; x < x_end; x+=1)
        {
            for(int y = y_start; y < y_end; ++y)
            {
                ivec2 tile_pos = {x, y};

                ivec2 zone_pos = {x/ZONE_SIZE, y/ZONE_SIZE};
                Zone *zone = game->level->zones + zone_pos[0] * game->level->zones_w_count + zone_pos[1];

                //Checking for pending work
                wid_t existing_work = workpool_get_at(&game->workpool, x, y);
                if(existing_work != WORK_INVALID)
                {
                    Work *work = workpool_get(&game->workpool, existing_work);
                    if(work->tile == cur_tile) continue;
                    workpool_remove(&game->workpool, existing_work);
                    zone->pending_build--;
                }
                //Checking for worker work
                {
                    int found = 0;
                    for(int i = 0; i < game->workers_count; ++i)
                    {
                        struct Worker *worker = game->workers + i;
                        if(worker->work == WORK_INVALID) continue;
                        Work *work = workpool_get(&game->workpool, worker->work);
                        if(work->position[0] == x && work->position[1] == y)
                        {
                            if(work->tile == cur_tile) { found = 1; break; }
                            workpool_remove_owned(&game->workpool, worker->work);
                            zone->pending_build--;
                            worker->work = WORK_INVALID;
                            break;
                        }
                    }
                    if (found) continue;
                }
                //Check tile under
                if(tilemap_get_tile(&game->level->tilemap, x, y) == cur_tile) continue;

                if(game->money < TILE_COST) continue;
                game->money -= TILE_COST;
                Error err = workpool_add_task(&game->workpool, work_create(tile_pos, cur_tile));
                if(err)
                {
                    LOG_ERROR("To many tasks !");
                    break;
                }
                else{
                    zone->pending_build++;
                }
            }
        }
        
    }
    ///////////

    //worker update
    
    if(is_mouse_released(frame->inputs, 0))
    {
        printf("9");
    }

    for(int i = 0; i < game->workers_count; ++i)
    {
        bt_tick(&game->worker_bt, game->w_runtimes+i, game->w_bb+i);
    }

    /*
    wid_t available_work = WORK_INVALID;
    for(int i = 0; i < game->workers_count; ++i)
    {
        struct Worker *worker = game->workers + i;
        if(available_work == WORK_INVALID && worker->work == WORK_INVALID) 
        {
            if(workpool_own_first(&game->workpool, &available_work) == ERR_OK)
            {
                worker->work = available_work;
            }
        }
        if(worker->work != WORK_INVALID)
        {
            Work *work = workpool_get(&game->workpool, worker->work);

            vec2 task_pos = {(float)work->position[0], (float)work->position[1]};
            vec2 dir;
            glm_vec2_sub(task_pos, worker->pos, dir);
            float target_dist = glm_vec2_norm(dir);
            if(target_dist < 0.5f)
            {
                Work *work = workpool_get(&game->workpool, worker->work);
                if(work->progression >= 1)
                {
                    workpool_remove_owned(&game->workpool, worker->work);
                    worker->work = WORK_INVALID;
                    tilemap_set_tile(&game->level->tilemap, work->tile, work->position[0], work->position[1]);
                }
                else
                {
                    work->progression += 0.5f * frame->dt;
                }
            }
            else
            {
                glm_vec2_normalize(dir);

                vec2 speed;
                glm_vec2_scale(dir, glm_min(WORKER_SPEED * frame->dt, target_dist), speed);

                glm_vec2_add(worker->pos, speed, worker->pos);
            }
        }
        else
        {
            vec2 speed;
            float dist = glm_vec2_norm(worker->pos);
            glm_vec2_normalize_to(worker->pos, speed);
            glm_vec2_scale(speed, -glm_min(frame->dt * WORKER_SPEED, dist), speed);
            glm_vec2_add(worker->pos, speed, worker->pos);
        }
    }
    */

    if(is_mouse_pressed(inputs, 1))
    {
        wid_t work_index;
        if(!workpool_own_first(&game->workpool, &work_index))
        {
            Work *work = workpool_get(&game->workpool, work_index);
            workpool_remove_owned(&game->workpool, work_index);
        }
        else { LOG_ERROR("No available tasks"); }
    }
    
    struct Level *level = game->level;
    vec2 levelpos = {0, 0};

    mat3 transform;
    vec2 pos = {0, 0};
    vec2 size = {1, 1};
    compute_transform(transform, pos, size);
    vec3 color = {1, 1, 1};

    tilemap_render_background(&level->tilemap, pos, 1);
    tilemap_render_layer(&level->tilemap, 0, pos, 1);

    mat3 debug_tran;
    vec2 pointer = {(float)(inputs.mouse_x/window.width), 1 - (float)(inputs.mouse_y/window.height)};
    camera_screen_to_world(&game->camera, pointer, pointer);
    compute_transform(debug_tran, pointer, (vec2){0.07f, 0.07f});
    draw_transformed_quad(shaders_use_default(), debug_tran, (float[]){1,1,1}, 1);


    //Draw build preview
    {
        unsigned int shader = shaders_use_atlas(get_atlas_tilemap(), cur_tile_x, cur_tile_y);

        for(int x = (int)bound_min[0]; x < (int)bound_max[0]; x+=1)
        {
            for(int y = (int)bound_min[1]; y < (int)bound_max[1]; ++y)
            {
                mat3 tile_transform;
                vec2 tile_pos = {(float)x+0.5f, (float)y+0.5f};
                compute_transform(tile_transform, tile_pos, (vec2){1,1});
                draw_transformed_quad(shader, tile_transform, (float[]){1, 1, 1}, 0.25f);
            }
        }
    }

    //Draw unassigned works 
    wid_t cur = game->workpool.head;
    while(cur != WORK_INVALID)
    {
        Work *work = workpool_get(&game->workpool, cur);
        mat3 work_transform; vec2 work_pos;

        work_pos[0] = (float)work->position[0] + 0.5f;
        work_pos[1] = (float)work->position[1] + 0.5f;

        compute_transform(work_transform, work_pos, (vec2){1,1});
        int tile_x, tile_y; 
        atlas_index_to_coordinates(get_atlas_tilemap(), work->tile-1, &tile_x, &tile_y);
        unsigned int shader = shaders_use_atlas(get_atlas_tilemap(), tile_x, tile_y);
        draw_transformed_quad(shader, work_transform, (float[]){1, 1, 1}, 0.25f);
        cur = game->workpool.nodes[cur].next;
    }

    //Draw worker works
    for(int i = 0; i < game->workers_count; ++i)
    {
        struct Worker *worker = game->workers + i;
        if(worker->work == WORK_INVALID) continue;
        Work *work = workpool_get(&game->workpool, worker->work);
        mat3 work_transform; vec2 work_pos;

        work_pos[0] = (float)work->position[0] + 0.5f;
        work_pos[1] = (float)work->position[1] + 0.5f;

        compute_transform(work_transform, work_pos, (vec2){1,1});
        int tile_x, tile_y; 
        atlas_index_to_coordinates(get_atlas_tilemap(), work->tile-1, &tile_x, &tile_y);
        unsigned int shader = shaders_use_atlas(get_atlas_tilemap(), tile_x, tile_y);
        draw_transformed_quad(shader, work_transform, (float[]){1, 1, 1}, 0.6f);
    }
    
    for(int i = 0; i < game->level->zones_count; ++i)
    {
        Zone *zone = game->level->zones + i;
        for(int b = 0; b < zone->box_count; ++b)
        {
            mat3 worker_transform; 
            compute_transform(worker_transform, (vec2){zone->pos[0] * ZONE_SIZE, zone->pos[1] * ZONE_SIZE}, (vec2){1,1});
            glm_translate2d(worker_transform, (vec3){0.5f + (float)(b/7) * 0.6f, 0.5f + 0.449f * (b-(b/7)*7), 0});
            draw_transformed_quad(shaders_use_sprite(get_texture_box()), worker_transform, (float[]){1, 1, 1}, 1);
        }
    }

    //Draw box 
    for(int i = 0; i < game->box_count; ++i)
    {
        vec2 pos;
        pos[0] = game->box[i][0];
        pos[1] = game->box[i][1];
        glm_vec2_add(pos, (vec2){0.5f, 0.5f}, pos);
        mat3 box_transform; 
        compute_transform(box_transform, pos, (vec2){1,1});
        draw_transformed_quad(shaders_use_sprite(get_texture_box()), box_transform, (float[]){1, 1, 1}, 1);
    }

    //Draw workers
    for(int i = 0; i < game->workers_count; ++i)
    {
        struct Worker *worker = game->workers + i;
        mat3 worker_transform; 
        compute_transform(worker_transform, worker->pos, (vec2){1,1});
        draw_transformed_quad(shaders_use_sprite(get_texture_worker()), worker_transform, (float[]){1, 1, 1}, 1);

        if(worker->is_holding_box)
        {
            mat3 worker_transform; 
            compute_transform(worker_transform, worker->pos, (vec2){1,1});
            glm_translate2d(worker_transform, (vec3){0.5f, 0.5f, 0});
            glm_rotate2d(worker_transform, 0.1f);
            draw_transformed_quad(shaders_use_sprite(get_texture_box()), worker_transform, (float[]){1, 1, 1}, 1);
        }
    }

    draw_ui(game, frame);

    igBegin("zones", 0, 0);

    for(int i = 0; i < game->level->zones_count; ++i)
    {
        Zone *zone = game->level->zones + i;
        igText("Zone %d", i);

        if(zone->pending_build > 0)
        {
            igText(" pending %d", zone->pending_build);
        }
        if(zone->workers_count > 0)
        {
            igText(" worker %d", zone->workers_count);
        }
        if(zone->box_count + zone->incoming_box_count > 0)
        {
            igText(" boxes %d(%d)", zone->box_count, zone->incoming_box_count);
        }
    }

    igEnd();

    igBegin("worker", 0, 0);

    for(int i = 0; i < game->workers_count; ++i)
    {
        BT_BlackBoard *bb = game->w_bb + i;
        if(bb->is_working)
        {
            igText("%d is working for zone %d", i, bb->assigned_zone);
        }
    }
    igEnd();

}
