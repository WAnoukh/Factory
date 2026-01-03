#include "game_runtime.h"
#include "game.h"
#include "GLFW/glfw3.h"
#include "engine.h"
#include "engine_commands.h"
#include "game_commands.h"
#include "level/level.h"
#include "memory/allocation.h"
#include "memory/arena.h"
#include "rendering/rendering.h"
#include "rendering/texture.h"
#include <assert.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_OPENGL3
#define CIMGUI_USE_GLFW
#include "cimgui.h"
#include "cimgui_impl.h"

void do_engine_setup(struct Game *game)
{
    engine_setup_imgui(game->engine);
    engine_setup_globals(game->engine);
    engine_register_commands();
    game_register_commands();
}

Error game_build(struct Game *game, GLFWwindow *window, struct Arena *arena)
{
    Error error = ERR_OK;
    ArenaOffset arena_offset = arena_get_offset(arena);
    *game = (struct Game){0};

    struct GameArenas *arenas;
    TRY_ALLOC(arenas, arena_allocate(arena, sizeof(struct GameArenas)),
            ERR_OOM, fail);
    arenas->main = arena;

    TRY_ALLOC(arenas->frame, arena_allocate(arena, sizeof(struct Arena)),
            ERR_OOM, fail);
    TRY_ALLOC(arenas->level, arena_allocate(arena, sizeof(struct Arena)),
            ERR_OOM, fail);

    if(
        !arena_create_patch(arena, MB(1), arenas->level) ||
        !arena_create_patch(arena, MB(1), arenas->frame)
      ) 
    { error = ERR_OOM; goto fail; }

    struct Engine *engine;
    TRY_ALLOC(engine, arena_allocate(arena, sizeof(struct Engine)),
            ERR_OOM, fail);

    TRY(engine_build(engine, window, arena), fail);

    struct TextureInfo *texture_info;
    TRY_ALLOC(texture_info, arena_allocate(arena, sizeof(struct TextureInfo)),
            ERR_OOM, fail);
    texture_set_info(texture_info);

    game->engine    = engine;
    game->arenas    = arenas;
    game->textures  = texture_info;

    return error;
fail:
    arena_set_offset(arena, arena_offset);
    return error;
}

Error game_runtime_start(struct Game *game)
{
    assert(game);
    assert(game->engine);
    assert(game->engine->rendering);
    Error error = ERR_OK;

    do_engine_setup(game);
    rendering_start(game->engine->rendering);
    load_default_images();
    console_set_command_context(game);
    game_start(game);

    return error;
}

void game_post_reload(struct Game *game)
{
    do_engine_setup(game);
    texture_set_info(game->textures);
    load_default_images();
}

void game_shutdown(struct Game *game)
{
    engine_shutdown(game->engine);
}

void game_runtime_update(struct Game *game, struct FrameContext frame)
{
    assert(game);
    assert(game->engine);
    struct Engine *engine = game->engine;

    engine_imgui_new_frame();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    game_update(game, &frame);

    engine_imgui_render(engine);

    glfwSwapBuffers(engine->window_ctx);
}

