#include "vec3.h"
#include "real.h"
#include <math.h>

vec3 vec3_init() {
    vec3 v = { 0, 0, 0 };
    return v;
}

vec3 vec3_init_values(real x, real y, real z) {
    vec3 v = { x, y, z };
    return v;
}

vec3 vec3_add(const vec3 *v1, const vec3 *v2) {
    return vec3_init_values(real_add(v1->x, v2->x), real_add(v1->y, v2->y), real_add(v1->z, v2->z));
}

vec3 vec3_subtract(const vec3 *v1, const vec3 *v2) {
    return vec3_init_values(real_sub(v1->x, v2->x), real_sub(v1->y, v2->y), real_sub(v1->z, v2->z));
}

vec3 vec3_multiply_scalar(const vec3 *v, real t) {
    return vec3_init_values(real_mul(v->x, t), real_mul(v->y, t), real_mul(v->z, t));
}

vec3 vec3_multiply_vec3(const vec3 *v1, const vec3 *v2) {
    return vec3_init_values(real_mul(v1->x, v2->x), real_mul(v1->y, v2->y), real_mul(v1->z, v2->z));
}

real vec3_length2(const vec3 *v) {
    return real_add(real_add(real_mul(v->x, v->x), real_mul(v->y, v->y)), real_mul(v->z, v->z));
}

real vec3_length(const vec3 *v) {
    return real_sqrt(vec3_length2(v));
}

vec3 vec3_normalized(const vec3 *v) {
    return vec3_multiply_scalar(v, real_sqrt_inv(vec3_length2(v)));
}

real vec3_dot(const vec3 *v1, const vec3 *v2) {
    return real_add(real_add(real_mul(v1->x, v2->x), real_mul(v1->y, v2->y)), real_mul(v1->z, v2->z));
}

ray ray_init(const vec3 *origin, const vec3 *direction) {
    ray r = { *origin, *direction };
    return r;
}
