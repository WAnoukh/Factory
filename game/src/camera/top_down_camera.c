#include "top_down_camera.h"
#include "GLFW/glfw3.h"
#include "cglm/io.h"
#include "cglm/util.h"
#include "cglm/vec2.h"
#include "game_runtime.h"
#include "inputs.h"
#include "rendering/camera.h"
#include <stdio.h>

void tdcamera_defaults(struct TDCamera *tdcamera)
{
    tdcamera->managed_cam = NULL;
    glm_vec2_zero(tdcamera->pan_speed);
}

void tdcamera_manage(struct TDCamera *tdcamera, struct Camera *camera)
{
    tdcamera->managed_cam = camera;
    camera->pos[1] = 0.5f;
    camera->zoom = 1;
}

void process_panning(struct TDCamera *tdcamera, struct FrameContext *frame);
void process_zooming(struct TDCamera *tdcamera, struct FrameContext *frame);

void tdcamera_update(struct TDCamera *tdcamera, struct FrameContext *frame)
{
    if(!tdcamera->managed_cam) return;
    struct Camera *cam = tdcamera->managed_cam;

    if(is_key_down(frame->inputs, GLFW_KEY_R))
    {
        glm_vec2_zero(cam->pos);
        glm_vec2_zero(tdcamera->pan_speed);
        cam->zoom = 1.f;
    }

    process_zooming(tdcamera, frame);
    process_panning(tdcamera, frame);
}

const float max_zoom = 0.5f;
const float min_zoom = 0.01f;
void process_zooming(struct TDCamera *tdcamera, struct FrameContext *frame)
{
    struct Camera *cam = tdcamera->managed_cam;

    float input = (float)(is_key_down(frame->inputs, GLFW_KEY_E) - is_key_down(frame->inputs, GLFW_KEY_Q)); 
    
    cam->zoom *= (1.f / (1 - input * 0.01f));
    cam->zoom = glm_clamp(cam->zoom, min_zoom, max_zoom);
}

const float pan_max_speed = 3.5f;
const float pan_acc = 5.f;
const float pan_dec = 10.5f;

void process_panning(struct TDCamera *tdcamera, struct FrameContext *frame)
{
    struct Camera *cam = tdcamera->managed_cam;

    vec2 speed; glm_vec2_copy(tdcamera->pan_speed, speed);
    vec2 input = {0, 0};

    input[1] = (float)(is_key_down(frame->inputs, GLFW_KEY_W) - is_key_down(frame->inputs, GLFW_KEY_S));
    input[0] = (float)(is_key_down(frame->inputs, GLFW_KEY_D) - is_key_down(frame->inputs, GLFW_KEY_A));

    glm_vec2_normalize(input);

    float speed_norm = glm_vec2_norm(speed);
    if(input[0] || input[1])
    {
        glm_vec2_scale(
                input, 
                glm_min(pan_max_speed, speed_norm + pan_acc * frame->real_dt), 
                speed 
                );
    }
    else
    {
        glm_vec2_normalize(speed);
        glm_vec2_scale(
                speed,
                glm_max(0, speed_norm - pan_dec * frame->real_dt),
                speed
                );
    }
    glm_vec2_copy(speed, tdcamera->pan_speed);
    glm_vec2_muladds(speed, frame->real_dt / cam->zoom, cam->pos);
}
