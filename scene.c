#include "scene.h"
#include "real.h"
#include "vec3.h"
#include "material.h"
#include <math.h>

#define FLOAT_INFINITY 0x75300000
#define EPSILON 0x00000147

real    scene_internal_ghost_radius;
real    scene_internal_ghost_radius2;
int     scene_internal_ghost_mode;
int     scene_internal_num_of_spheres;
sphere *scene_internal_marked_sphere_address;

sphere scene_spheres[MAX_SPHERES];

real * const scene_ghost_radius = &scene_internal_ghost_radius;
real * const scene_ghost_radius2 = &scene_internal_ghost_radius2;
int * const scene_ghost_mode = &scene_internal_ghost_mode;
int * const scene_num_of_spheres = &scene_internal_num_of_spheres;
sphere ** const scene_marked_sphere_address = &scene_internal_marked_sphere_address;

static real scene_clamp_real(real x) {
    if (x < 0) {
        return 0;
    }
    if (x >= 0x00010000) {
        return 0x00010000;
    }
    return x;
}

static vec3 scene_clamp_vec3(const vec3 *color) {
    vec3 result;
    result.x = scene_clamp_real(color->x);
    result.y = scene_clamp_real(color->y);
    result.z = scene_clamp_real(color->z);
    return result;
}

static real real_mod2(real x) {
    return x & 1;
}

vec3 scene_get_sky_color(real direction_y) {
    const int n = 2;
    const int offset = ((0x00010000 << n) - 0x00010000);
    const int m = 1;
    int y = scene_clamp_real(direction_y << m);
    real k = (y + offset) >> n;

    vec3 sky_color = material_color_sky[material_light_mode];
    vec3 temp2 = vec3_multiply_scalar(&sky_color, k);
    k = real_sub(0x0000ffff, k);
    vec3 horizon_color = material_color_horizon[material_light_mode];
    vec3 temp1 = vec3_multiply_scalar(&horizon_color, k);
    vec3 result_color = vec3_add(&temp1, &temp2);

    // Return the interpolated color
    return result_color;
}

/*
 * P(t) = origin.y + t*direction.y = 0 , jer ide do poda, y = 0 -> racunamo samo u y smeru
 * 0 = origin.y + t*direction.y
 * t = -origin.y/direction.y        , koliko daleko treba ici u smeru direction da bi y postalo 0, tj. da dodje do preseka
 */

static real real_div_cheap(real x, real y) {
    real inv_y = y;
    int i = 0;

compute_inverse:
    inv_y = real_mul(inv_y, (0x00020000 - real_mul(inv_y, y)));
    if (++i < 16) goto compute_inverse;

    return real_mul(x, inv_y);
}

static int scene_intersect_plane(real origin_y, real direction_y, real *t) {
    if (direction_y == 0) {
        return 0;
    }
    int intersects = (origin_y ^ direction_y) & 0x80000000;
    if (!intersects) {
        return 0;
    }

    *t = -real_div_cheap(origin_y, direction_y);
    return 1;
}



static real transform(real t) {
    if (t < 0) {
        return 0;
    }
    t = t >> 7;
    t = real_mul(t, t);
    t = 0x0000ffff - t;
    if (t < 0) {
        return 0;
    }
    return t;
}

static vec3 scene_get_floor_color(real x, real z, real t) {
    x = int_real(x);
    z = int_real(z);
    vec3 base1 = material_color_floor1[material_light_mode];
    vec3 base2 = material_color_floor2[material_light_mode];
    vec3 base = real_mod2(x + z) ? base1 : base2;
    real k = transform(t);
    vec3 temp1 = vec3_multiply_scalar(&base, k);
    k = 0x0000ffff - k;
    vec3 mid = material_color_floor_average[material_light_mode];
    vec3 temp2 = vec3_multiply_scalar(&mid, k);
    return vec3_add(&temp1, &temp2);
}

vec3 scene_trace_ray(const ray *r, int depth) {
    if (depth >= MAX_DEPTH) {
        return scene_get_sky_color(r->direction.y);
    }

    real t_closest = FLOAT_INFINITY;
    const sphere *closest_sphere = NULL;
    int hit_plane = 0;
    real t_plane = FLOAT_INFINITY;

    for (int i = 0; i < *scene_num_of_spheres; ++i) {
        real t = 0;
        if (sphere_intersect(&scene_spheres[i], r, &t) && t < t_closest) {
            t_closest = t;
            closest_sphere = &scene_spheres[i];
        }
    }

    if (scene_intersect_plane(r->origin.y, r->direction.y, &t_plane) && !closest_sphere) {
        t_closest = t_plane;
        closest_sphere = NULL;
        hit_plane = 1;
    }

    vec3 temp0, temp1, temp2;

    if (closest_sphere) {
        goto scene_trace_ray_continue;
    }
    if (hit_plane) {
        goto scene_trace_ray_continue;
    }
    return scene_get_sky_color(r->direction.y);
scene_trace_ray_continue:
    t_closest -= EPSILON;

    temp0 = vec3_multiply_scalar(&r->direction, t_closest);
        
    vec3 hit_point = vec3_add(
        &r->origin,
        &temp0
    );
        
    material mat;
    vec3 reflected_direction;
    if (hit_plane) {
        vec3 temp;
        temp = scene_get_floor_color(hit_point.x, hit_point.z, t_closest);
        mat.color = temp;
        mat.reflectivity = 0x00003333;
        reflected_direction = r->direction;
        reflected_direction.y = ~reflected_direction.y;
    }
    else {
        temp0 = vec3_subtract(&hit_point, &closest_sphere->center);
        vec3 normal = vec3_normalized(&temp0);
        mat = closest_sphere->mat;
        real scalar = vec3_dot(&r->direction, &normal);
        scalar <<= 1;
        temp0 = vec3_multiply_scalar(&normal, scalar);
        reflected_direction = vec3_subtract(
            &r->direction,
            &temp0
        );
    }
    vec3 reflected_color = scene_trace_ray(&(ray){hit_point, reflected_direction}, depth + 1);
    temp0 = vec3_multiply_scalar(&mat.color, real_sub(0x00010000, mat.reflectivity));
    temp1 = vec3_multiply_scalar(&reflected_color, mat.reflectivity);
    temp2 = vec3_add(
        &temp0,
        &temp1
    );
    return scene_clamp_vec3(&temp2);
}

void scene_reset() {
    camera_init();
    *scene_num_of_spheres = 0;
    *scene_marked_sphere_address = 0;
    *scene_ghost_mode = 0;
}

static int index;

void scene_add_sphere(const vec3 *position, real radius) {
    if (*scene_num_of_spheres >= MAX_SPHERES) {
        return;
    }
    if (position->y < radius) {
        return;
    }
    for (int i = 0; i < *scene_num_of_spheres; i++) {
        vec3 pointer = vec3_subtract(position, &scene_spheres[i].center);
        real min_distance = radius + scene_spheres[i].radius;
        real min_distance2 = real_mul(min_distance, min_distance);
        if (vec3_length2(&pointer) < min_distance2) {
            return;
        }
    }
    sphere new_sphere;
    new_sphere.center = *position;
    new_sphere.radius = radius;
    new_sphere.radius2 = real_mul(radius, radius);
        
    real reflexivities[] = {
        0x00008000,
        0x00008000,
        0x00008000,
        0x00008000
    };

    vec3 color = material_color_sphere[index];
    real reflectivity = reflexivities[index];
    index = (index + 1) % 4;

    new_sphere.mat.color = color;
    new_sphere.mat.reflectivity = reflectivity;
    scene_spheres[(*scene_num_of_spheres)++] = new_sphere;
}

void scene_remove_sphere(sphere *sp) {
    if (!sp) {
        return;
    }
    for (int i = 0; i < *scene_num_of_spheres; i++) {
        if (&scene_spheres[i] == sp) {
            scene_spheres[i] = scene_spheres[--(*scene_num_of_spheres)];
            return;
        }
    }
}
