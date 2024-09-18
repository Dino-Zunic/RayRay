#pragma once

#include "vec3.h"

#define CAMERA_WIDTH 800        //sirina ekrana
#define CAMERA_HEIGHT 600

typedef struct {
    vec3 position;
    real pitch;
    real yaw;
    real cos_pitch;
    real sin_pitch;
    real cos_yaw;
    real sin_yaw;
} camera;

camera camera_init();

void camera_setup(camera *cam);
void camera_update_angles(camera *cam);

vec3 camera_get_forward(const camera *cam);
vec3 camera_get_right(const camera *cam);
vec3 camera_get_up(const camera *cam);
vec3 camera_get_ray_direction(camera *cam, real u, real v);
real camera_get_pitch(const camera *cam);
real camera_get_yaw(const camera *cam);
real camera_get_sin_pitch(const camera *cam);
real camera_get_cos_pitch(const camera *cam);
vec3 camera_get_position(const camera *cam);

void camera_set_position(camera *cam, const vec3 *position);
void camera_set_pitch(camera *cam, real pitch);
void camera_set_yaw(camera *cam, real yaw);
