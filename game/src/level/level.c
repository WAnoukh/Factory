#include "level.h"
#include "memory/arena.h"
#include "rendering/rendering.h"
#include "tilemap/tilemap.h"
#include <string.h>

struct Level *level_create_empty(int w, int h, struct Arena *arena)
{
    char *arena_offset = arena_get_offset(arena);

    struct Level *level = arena_allocate(arena, sizeof(struct Level));
    if(!level) goto cleanup;
    
    int tile_count = w * h;
    level->tilemap.tile = arena_allocate_align(arena, sizeof(Tile)*tile_count, alignof(Tile));
    if(!level->tilemap.tile) goto cleanup;

    memset(level->tilemap.tile, 0, sizeof(Tile)*tile_count);
    level->tilemap.width = w;
    level->tilemap.height = h;

    return level;
cleanup:
    arena_set_offset(arena, arena_offset);
    return NULL;
}

void level_render(struct Level *level)
{
    unsigned int shader = shaders_use_default(); 
    if(shader)
    {

    }
}

void level_world_to_tile(struct Level *level, vec2 world_pos, ivec2 tile_pos)
{
    tile_pos[0] = (int)world_pos[0];
    tile_pos[1] = (int)world_pos[1];
}
