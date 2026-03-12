#ifndef LEVEL_H
#define LEVEL_H

#include "tilemap/tilemap.h"

#define ZONE_SIZE 5
#define MAP_SIZE 50

struct Arena;

typedef struct Zone{
    ivec2   pos;
    int     box_count;
    int     incoming_box_count;
    int     workers_count;
    int     pending_build;
} Zone;

struct Level 
{
    struct TileMap  tilemap; 
    Zone           *zones; 
    int             zones_count;
    int             zones_w_count;
};

struct Level *level_create_empty(int w, int h, struct Arena *arena);

void level_render(struct Level *level);

void level_world_to_tile(struct Level *level, vec2 world_pos, ivec2 tile_pos);

#endif // LEVEL_H
