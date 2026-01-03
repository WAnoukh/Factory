#ifndef LEVEL_H
#define LEVEL_H

#include "tilemap/tilemap.h"

struct Arena;

struct Level 
{
   struct TileMap tilemap; 
};

struct Level *level_create_empty(int w, int h, struct Arena *arena);

void level_render(struct Level *level);

void level_world_to_tile(struct Level *level, vec2 world_pos, ivec2 tile_pos);

#endif // LEVEL_H
