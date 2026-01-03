#include <cglm/cglm.h>
#include <stdio.h>

#include "glad/glad.h"
#include "rendering/camera.h"
#include "rendering/shader.h"
#include "texture.h"
#include "rendering/rendering.h"

#define GLCheckError() GLCheckErrorImpl(__FILE__, __LINE__)

struct Rendering *current_rendering = NULL;

static void GLCheckErrorImpl(const char* file, int line) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        const char* error;
        switch (err) {
            case GL_INVALID_ENUM:                  error = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               error = "Unknown Error"; break;
        }
        fprintf(stderr, "OpenGL error (%s) in %s:%d\n", error, file, line);
    }
}

float triangle_vertices[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    0.0f, 0.5f, 0.0f
};

float quad_vertices[] = {
     // positions          //texture coords
     0.5f,  0.5f,  0.0f,   1.0f, 1.0f, 
     0.5f, -0.5f,  0.0f,   1.0f, 0.0f,
    -0.5f, -0.5f,  0.0f,   0.0f, 0.0f,
    -0.5f,  0.5f,  0.0f,   0.0f, 1.0f
};

unsigned int quad_indices[] = {
    // note that we start from 0!
    0, 1, 3, // first triangle
    1, 2, 3 // second triangle
};

void rendering_set_current(struct Rendering *info)
{
    current_rendering = info;
}

void load_default_shaders()
{
    assert(current_rendering);
    current_rendering->shader_program_default = create_shader_program("resources/shader/generic/default.vert", "resources/shader/generic/default.frag");
    if (!current_rendering->shader_program_default)
    {
        exit(1);
    }
    current_rendering->shader_program_sprite = create_shader_program("resources/shader/generic/sprite.vert","resources/shader/generic/sprite.frag");
    if(!current_rendering->shader_program_sprite)
    {
        exit(1);
    }
    current_rendering->shader_program_atlas = create_shader_program("resources/shader/generic/atlas.vert","resources/shader/generic/atlas.frag");
    if(!current_rendering->shader_program_atlas)
    {
        exit(1);
    }
}


void create_screen_space_view(mat3 out_view)
{
    glm_mat3_zero(out_view);
    out_view[0][0] = 2;
    out_view[1][1] = 2;
    out_view[2][0] = -1;
    out_view[2][1] = -1;
}

void rendering_set_camera(struct Camera *new_main_camera)
{
    assert(current_rendering);
    assert(new_main_camera);
    current_rendering->main_camera = new_main_camera;
}

void rendering_defaults(struct Rendering *rendering)
{
    *rendering = (struct Rendering){0};
    create_screen_space_view(rendering->screen_space_view);
}

void load_quad_gpu(struct Rendering *rendering)
{
    glGenVertexArrays(1, &rendering->quad_VAO);
    glGenBuffers(1, &rendering->quad_VBO);
    glGenBuffers(1, &rendering->quad_EBO);

    glBindVertexArray(rendering->quad_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, rendering->quad_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendering->quad_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

void load_triangle_gpu(struct Rendering *rendering)
{
    glGenVertexArrays(1, &rendering->triangle_VAO);
    glGenBuffers(1, &rendering->triangle_VBO);

    glBindVertexArray(rendering->triangle_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, rendering->triangle_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

void rendering_start(struct Rendering *rendering)
{
    load_quad_gpu(rendering);
    load_triangle_gpu(rendering);
    load_default_shaders();
}

unsigned int shaders_use_default()
{
    assert(current_rendering);
    assert(current_rendering->shader_program_default);
    glUseProgram(current_rendering->shader_program_default);
    return current_rendering->shader_program_default;
}
 
unsigned int shaders_use_sprite(unsigned int texture)
{
    assert(current_rendering);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(current_rendering->shader_program_sprite);
    shader_set_int(current_rendering->shader_program_sprite, "texture", 0);
    return current_rendering->shader_program_sprite;
}

unsigned int shaders_use_atlas(struct TextureAtlas atlas, int x, int y)
{
    assert(current_rendering);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas.texture_id);
    glUseProgram(current_rendering->shader_program_atlas);
    GLCheckError();
    shader_set_int(current_rendering->shader_program_atlas, "texture", 0);
    GLCheckError();
    shader_set_vec2(current_rendering->shader_program_atlas, "atlas_pos", (float)x, (float)y);
    GLCheckError();
    shader_set_vec2(current_rendering->shader_program_atlas, "atlas_size", (float)atlas.width, (float)atlas.height);
    GLCheckError();
    return current_rendering->shader_program_atlas;
}

void draw_quad()
{
    assert(current_rendering);
    assert(current_rendering->quad_EBO);

    glBindVertexArray(current_rendering->quad_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    GLCheckError();
}

void draw_wire_quad()
{
    assert(current_rendering);
    assert(current_rendering->quad_EBO);

    glBindVertexArray(current_rendering->quad_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    GLCheckError();
}

void draw_transformed_quad(unsigned int program, mat3 transform, vec3 color, float alpha)
{
    assert(current_rendering);
    assert(current_rendering->main_camera);
    mat3 result;
    glm_mat3_mul(current_rendering->main_camera->view, transform, result);
    //glm_mat3_mul(transform, camera->view, result);
    shader_set_mat3(program, "view", result);
    shader_set_vec3(program, "color", color);
    shader_set_float(program, "alpha", alpha);
    draw_quad();
}

void draw_transformed_quad_screen_space(unsigned int program, mat3 transform, vec3 color, float alpha)
{
    assert(current_rendering);
    mat3 result;
    glm_mat3_mul(current_rendering->screen_space_view, transform, result);
    //glm_mat3_mul(transform, camera->view, result);
    shader_set_mat3(program, "view", result);
    shader_set_vec3(program, "color", color);
    shader_set_float(program, "alpha", alpha);
    draw_quad();
}

void render_triangle()
{
    assert(current_rendering);
    assert(current_rendering->triangle_VBO);
    assert(current_rendering->shader_program_default);

    glUseProgram(current_rendering->shader_program_default);
    glBindVertexArray(current_rendering->triangle_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
