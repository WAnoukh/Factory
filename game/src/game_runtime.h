#ifndef GAME_RUNTIME_H
#define GAME_RUNTIME_H

#include "error/error.h"
#include "inputs.h"
#include "window_context.h"
typedef struct GLFWwindow GLFWwindow;
struct Game;
struct InputInfo;

struct GameArenas
{
    struct Arena *main;
    struct Arena *frame;
    struct Arena *level;
};

struct FrameContext
{
    struct WindowContext    window;
    struct InputInfo        inputs;
    float                   dt;
    float                   time;
    float                   real_dt;
    float                   real_time;
};

Error game_build(struct Game *game, GLFWwindow *window, struct Arena *arena);

Error game_runtime_start(struct Game *game);

void game_shutdown(struct Game *game);

void game_post_reload(struct Game *game);

void game_runtime_update(struct Game *game, struct FrameContext frame);

#endif // GAME_RUNTIME_H
