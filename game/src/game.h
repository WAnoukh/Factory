#ifndef GAME_H
#define GAME_H

#define BOX_MAX 100

#include "camera/top_down_camera.h"
#include "gameplay/workpool.h"
#include "rendering/camera.h"

typedef struct GLFWwindow GLFWwindow;
struct GameArenas;
struct Engine;
struct Level;
struct InputInfo;
struct FrameContext;
struct Worker;

struct Game 
{
    struct TDCamera     tdcamera;
    struct Camera       camera;
    WorkPool            workpool;

    struct GameArenas  *arenas;
    struct TextureInfo *textures;
    struct Engine      *engine;
    struct Level       *level;

    struct Worker  *workers;
    int             workers_count;

    vec2    box[BOX_MAX];
    int     box_count;

    float   time_scale;
    int     money;
};

void game_start(struct Game *game);

void game_update(struct Game *game, struct FrameContext *frame);

#endif // GAME_H
