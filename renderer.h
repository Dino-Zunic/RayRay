#pragma once

#include "scene.h"

typedef struct {
    vec3 canvas[CAMERA_HEIGHT][CAMERA_WIDTH];
} renderer;

void renderer_render(renderer *r, scene *sc);
vec3 renderer_get_color(const renderer *r, int i, int j);
void renderer_sort_spheres(const sphere *spheres[], int num_of_spheres, const camera *cam);
void rendered_render_sphere_projected(renderer *r, const sphere *s, const camera *cam);
void renderer_render_sphere(renderer *r, const sphere *s, const camera *cam);
void renderer_render_floor(renderer *r, const camera *cam);
void renderer_calculate_bounds_x(real a, real b, real c, real A, real *x_min, real *x_max);
void renderer_solve_quadratic(real a, real b, real c, real *x_min, real *x_max);
