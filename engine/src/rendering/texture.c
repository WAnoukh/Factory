#define STB_IMAGE_IMPLEMENTATION
#include "glad/glad.h"
#include <stb_image.h>

#include "texture.h"

struct TextureInfo *textureinfo;

void texture_set_info(struct TextureInfo *info)
{
    textureinfo = info;
};

struct TextureAtlas get_texture_font_atlas()
{
    assert(textureinfo);
    return textureinfo->atlas_font;
}

struct TextureAtlas get_atlas_tilemap()
{
    assert(textureinfo);
    return textureinfo->atlas_tilemap;
}

unsigned int get_texture_worker()
{
    assert(textureinfo);
    return textureinfo->texture_worker;
}

GLuint texture_load(char* path)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 4);
    if(data != NULL)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        printf( "Failed to load texture %s\n", path);
        return 0;
    }
    stbi_image_free(data);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texture;
}

void load_default_images()
{
    assert(textureinfo);
    textureinfo->atlas_font.texture_id = texture_load("resources/sprite/font.png");
    textureinfo->atlas_font.width = 10;
    textureinfo->atlas_font.height = 5;
    textureinfo->atlas_tilemap.texture_id = texture_load("resources/sprite/TileMap.png");
    textureinfo->atlas_tilemap.width = 16;
    textureinfo->atlas_tilemap.height = 16;
    textureinfo->texture_worker = texture_load("resources/sprite/worker.png");
}

