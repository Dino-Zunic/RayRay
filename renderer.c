#include "renderer.h"
#include "material.h"
#include <math.h>
#include <stdlib.h>

void renderer_render(renderer *r, scene *sc) {
    const sphere *spheres[MAX_SPHERES];
    int num_of_spheres = scene_get_num_of_spheres(sc);
    for (int i = 0; i < num_of_spheres; i++) {
        spheres[i] = &scene_get_spheres(sc)[i];
    }
    renderer_sort_spheres(spheres, num_of_spheres, scene_get_camera(sc));
    vec3 sky_color = material_color_sky[material_light_mode];
    for (int i = 0; i < CAMERA_HEIGHT; i++) {
        for (int j = 0; j < CAMERA_WIDTH; j++) {
            r->canvas[i][j] = sky_color;
        }
    }
    renderer_render_floor(r, scene_get_camera(sc));
    for (int i = 0; i < num_of_spheres; i++) {
        if (spheres[i] == sc->marked) {
            sphere s = *spheres[i];
            s.mat.color = material_color_rgb(0xff0000);
            renderer_render_sphere(r, &s, scene_get_camera(sc));
        }
        else {
            renderer_render_sphere(r, spheres[i], scene_get_camera(sc));
        }
    }
    if (sc->ghost_mode) {
        sphere ghost_projected = sphere_init_with_values(
            vec3_init_values(0, 0, SCENE_SPAWN_DISTANCE + sc->ghost_radius),
            sc->ghost_radius, 
            material_init_with_values(material_color_rgb(0xFF0000), 0x00010000)
        );
        rendered_render_sphere_projected(r, &ghost_projected, &sc->cam);
    }
}

vec3 renderer_get_color(const renderer *r, int i, int j) {
    return r->canvas[i][j];
}

void renderer_sort_spheres(const sphere *spheres[], int num_of_spheres, const camera *cam) {
    int pos = num_of_spheres;
    while (pos != 0) {
        int bound = pos;
        vec3 temp;
        pos = 0;
        for (int i = 0; i < bound - 1; i++) {
            vec3 cam_pos = camera_get_position(cam);
            vec3 temp;
            temp = vec3_subtract(&spheres[i]->center, &cam_pos);
            real dist1 = vec3_length2(&temp);
            temp = vec3_subtract(&spheres[i + 1]->center, &cam_pos);
            real dist2 = vec3_length2(&temp);
            if (dist1 < dist2) {
                const sphere *temp = spheres[i];
                spheres[i] = spheres[i + 1];
                spheres[i + 1] = temp;
                pos = i + 1;
            }
        }
    }
}

void rendered_render_sphere_projected(renderer *r, const sphere *s, const camera *cam) {
    vec3 c = s->center;

    if (c.z < 0) {
        return;
    }

    real A = vec3_length2(&c) - real_mul(s->radius, s->radius);

    real x_min, x_max, y_min, y_max;
    renderer_calculate_bounds_x(c.x, c.y, c.z, A, &x_min, &x_max);
    renderer_calculate_bounds_x(c.y, c.x, c.z, A, &y_min, &y_max);

    int i_min = int_real(y_min * CAMERA_HEIGHT) + CAMERA_HEIGHT / 2;
    int i_max = int_real(y_max * CAMERA_HEIGHT) + CAMERA_HEIGHT / 2;
    int j_min = int_real(x_min * CAMERA_HEIGHT) + CAMERA_WIDTH / 2;
    int j_max = int_real(x_max * CAMERA_HEIGHT) + CAMERA_WIDTH / 2;

    if (i_min < 0) { i_min = 0; }
    if (j_min < 0) { j_min = 0; }
    if (i_max > CAMERA_HEIGHT - 1) { i_max = CAMERA_HEIGHT - 1; }
    if (j_max > CAMERA_WIDTH - 1) { j_max = CAMERA_WIDTH - 1; }

    if (1) { i_min = 0; }
    if (1) { j_min = 0; }
    if (1) { i_max = CAMERA_HEIGHT - 1; }
    if (1) { j_max = CAMERA_WIDTH - 1; }

    for (int y = i_min; y <= i_max; y++) {
        for (int x = j_min; x <= j_max; x++) {
            const real rx = real_int(x - 400) >> 9;
            const real ry = real_int(y - 300) >> 9;
            const real temp1 = real_mul(c.x, rx) + real_mul(c.y, ry) + c.z;
            if (real_mul(temp1, temp1) >= real_mul(A, real_mul(rx, rx) + real_mul(ry, ry) + 0x00010000)) {
                r->canvas[y][x] = s->mat.color;
            }
        }
    }
}

void renderer_render_sphere(renderer *r, const sphere *s, const camera *cam) {
    vec3 cam_pos = camera_get_position(cam);
    vec3 camera_to_sphere = vec3_subtract(&s->center, &cam_pos);
    vec3 right = camera_get_right(cam);
    vec3 up = camera_get_up(cam);
    vec3 forward = camera_get_forward(cam);
    vec3 c = vec3_init_values(
        vec3_dot(&camera_to_sphere, &right),
        vec3_dot(&camera_to_sphere, &up),
        vec3_dot(&camera_to_sphere, &forward)
    );

    sphere projected = sphere_init_with_values(c, s->radius, s->mat);
    rendered_render_sphere_projected(r, &projected, cam);
}

void renderer_render_floor(renderer *r, const camera *cam) {
    vec3 floor_color = material_color_floor1[material_light_mode];
    real c = camera_get_cos_pitch(cam);
    real s = camera_get_sin_pitch(cam);
    for (int y = 0; y < CAMERA_HEIGHT; y++) {
        for (int x = 0; x < CAMERA_WIDTH; x++) {
            if (c * (299 - y) > (s << 9)) {
                r->canvas[y][x] = floor_color;
            }
        }
    }
}

// <privremena funkcija>
static void renderer_solveQuadratic(float a, float b, float c, real *xMin, real *xMax) {
    if (a == 0) {
        *xMin = *xMax = real_float(-c / b);
        return;
    }
    float discriminant = b * b - 4 * a * c;
    float delta = sqrtf(discriminant);
    if (a > 0) {
        *xMin = real_float((-b - delta) / 2 / a);
        *xMax = real_float((-b + delta) / 2 / a);
    }
    else {
        *xMin = real_float((-b + delta) / 2 / a);
        *xMax = real_float((-b - delta) / 2 / a);
    }
}

void renderer_calculate_bounds_x(real a, real b, real c, real A, real *x_min, real *x_max) {
    // <privremeno>

    float a_ = float_real(a), b_ = float_real(b), c_ = float_real(c), A_ = float_real(A);
    float a2_ = a_ * a_, b2_ = b_ * b_, c2_ = c_ * c_;
    renderer_solveQuadratic(
        a2_ * b2_ - (b2_ - A_) * (a2_ - A_),
        2 * a_ * c_ * A_,
        b2_ * c2_ - (b2_ - A_) * (c2_ - A_),
        x_min,
        x_max
    );
}
