#include "game.h"
#include "GLFW/glfw3.h"
#include "camera/top_down_camera.h"
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

void game_start(struct Game *game)
{
    game->time_scale = 1;
    game->camera = camera_get_default();
    rendering_set_camera(&game->camera);
    game->level = level_create_empty(20, 20, game->arenas->level);
    tilemap_fill(&game->level->tilemap, 1);
    game->level->tilemap.tile[0] = 2;
    tdcamera_defaults(&game->tdcamera);
    tdcamera_manage(&game->tdcamera, &game->camera);
    
    workpool_defaults(&game->workpool);

    game->workers_count = 5;
    game->workers = arena_allocate(game->arenas->main, sizeof(struct Worker) * WORKERS_MAX);
    if(!game->workers)
    {
        assert(0); abort();
    }
    for(int i = 0; i < game->workers_count; ++i)
    {
        worker_defaults(game->workers + i);
    }

    game->money = 10000;
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

                //Checking for pending work
                wid_t existing_work = workpool_get_at(&game->workpool, x, y);
                if(existing_work != WORK_INVALID)
                {
                    Work *work = workpool_get(&game->workpool, existing_work);
                    if(work->tile == cur_tile) continue;
                    workpool_remove(&game->workpool, existing_work);

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
            }
        }
        
    }
    ///////////

    //worker update
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

    //Draw workers
    for(int i = 0; i < game->workers_count; ++i)
    {
        struct Worker *worker = game->workers + i;
        mat3 worker_transform; 
        compute_transform(worker_transform, worker->pos, (vec2){1,1});
        draw_transformed_quad(shaders_use_sprite(get_texture_worker()), worker_transform, (float[]){1, 1, 1}, 1);
    }

    draw_ui(game, frame);
}
