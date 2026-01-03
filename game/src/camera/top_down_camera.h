#ifndef TOP_DOWN_CAMERA_H
#define TOP_DOWN_CAMERA_H

#include "cglm/types.h"
struct Arena;
struct FrameContext;

struct TDCamera
{
    struct Camera * managed_cam;
    vec2 pan_speed;
    float zoom_speed;
};

void tdcamera_defaults(struct TDCamera *tdcamera);

void tdcamera_manage(struct TDCamera *tdcamera, struct Camera *camera);

void tdcamera_update(struct TDCamera *tdcamera, struct FrameContext *frame);

#endif // TOP_DOWN_CAMERA_H
