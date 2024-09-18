#include "camera.h"
#include "real.h"
#include "vec3.h"
#include <math.h>

camera camera_init() {
    camera cam;
    camera_setup(&cam);
    return cam;
}

void camera_setup(camera *cam) {
    cam->position = vec3_init_values(real_int(0), real_int(3), real_int(0));
    cam->pitch = real_int(0);
    cam->yaw = real_int(0);
    camera_update_angles(cam);
}

void camera_update_angles(camera *cam) {
    cam->cos_pitch = real_cos(cam->pitch);
    cam->sin_pitch = real_sin(cam->pitch);
    cam->cos_yaw = real_cos(cam->yaw);
    cam->sin_yaw = real_sin(cam->yaw);
}

vec3 camera_get_forward(const camera *cam) {                        //od kamere do sredine ekrana
    vec3 forward = vec3_init_values(
        real_mul(cam->cos_pitch, cam->cos_yaw),
        cam->sin_pitch,
        real_mul(cam->cos_pitch, cam->sin_yaw)
    );
    return forward;
}

vec3 camera_get_right(const camera *cam) {                          //desno od kamere, a paralelno sa ekranom (koji je 1.0f udaljen od kamere)
    vec3 right = vec3_init_values(cam->sin_yaw, 0, -cam->cos_yaw);
    return right;
}

vec3 camera_get_up(const camera *cam) {                             //gore -||-
    vec3 up = vec3_init_values(
        ~real_mul(cam->sin_pitch, cam->cos_yaw),
        cam->cos_pitch,
        ~real_mul(cam->sin_pitch, cam->sin_yaw)
    );
    return up;
}

vec3 camera_get_ray_direction(camera *cam, real u, real v) {        //poziva se za svaki piksel ekrana
    vec3 forward = camera_get_forward(cam);
    vec3 right = camera_get_right(cam);
    vec3 up = camera_get_up(cam);

    vec3 right_scaled = vec3_multiply_scalar(&right, u);
    vec3 up_scaled = vec3_multiply_scalar(&up, v);

    vec3 temp = vec3_add(&forward, &right_scaled);
    vec3 direction = vec3_add(&temp, &up_scaled);

    return vec3_normalized(&direction);
}

real camera_get_pitch(const camera *cam) {
    return cam->pitch;
}

real camera_get_yaw(const camera *cam) {
    return cam->yaw;
}

real camera_get_sin_pitch(const camera *cam) {
    return cam->sin_pitch;
}

real camera_get_cos_pitch(const camera *cam) {
    return cam->cos_pitch;
}

vec3 camera_get_position(const camera *cam) {
    return cam->position;
}

void camera_set_position(camera *cam, const vec3 *position) {
    cam->position = *position;
}

void camera_set_pitch(camera *cam, real pitch) {
    cam->pitch = pitch;
}

void camera_set_yaw(camera *cam, real yaw) {
    cam->yaw = yaw;
}
