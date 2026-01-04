#ifndef TILEMAP_H
#define TILEMAP_H
#include <cglm/ivec2.h>

typedef int Tile;

struct TileMap
{
    Tile *tile;
    int width;
    int height;
    int layer_count;
};

static inline Tile *tilemap_get_layer_by_index(struct TileMap *tilemap, int index)
{
    if(index < 0 || index >= tilemap->layer_count)
    {
        return NULL;
    }
    return tilemap->tile+(index * tilemap->width * tilemap->height);
}

void tilemap_render_background(const struct TileMap *tilemap, vec2 pos, float size);

void tilemap_render_layer(struct TileMap *tilemap, int layer, vec2 pos, float size);

void tilemap_shift_right(struct TileMap *tilemap, int amount);

void tilemap_shift_left(struct TileMap *tilemap, int amount);

void tilemap_shift_up(struct TileMap *tilemap, int amount);

void tilemap_shift_down(struct TileMap *tilemap, int amount);

int tilemap_get_tile(struct TileMap *tilemap, int x, int y);

void tilemap_set_tile(struct TileMap *tilemap, int tile, int x, int y);

void tilemap_fill(struct TileMap *tilemap, int tile);

#endif
