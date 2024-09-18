#pragma once

#include "sphere.h"
#include "rng.h"
#include "camera.h"
#include "material.h"

#define MAX_DEPTH 5                     //MAX_DEPTH - koliko puta pratimo odbijanje zraka
#define MAX_SPHERES 50
#define SCENE_SPAWN_DISTANCE 0x00010000

typedef struct {
    sphere spheres[MAX_SPHERES];
    real ghost_radius;
    int ghost_mode;
    int num_of_spheres;
    sphere *marked;
    rng random_gen;
    camera cam;
} scene;

const sphere *scene_get_spheres(const scene *sc);
int scene_get_num_of_spheres(const scene *sc);
rng *scene_get_rng(scene *sc);
camera *scene_get_camera(scene *sc);

real scene_clamp_real(real x);
vec3 scene_clamp_vec3(const vec3 *color);

int scene_intersect_plane(const ray *r, real *t);
vec3 scene_get_floor_color(const vec3 *point, real t);
vec3 scene_trace_ray(scene *sc, const ray *r, int depth);

void scene_reset(scene *sc);

vec3 get_sky_color(const ray *r);

scene *get_scene();

void scene_add_sphere(scene *sc, const vec3 *position, real radius);
void scene_remove_sphere(scene *sc, sphere *sp);
