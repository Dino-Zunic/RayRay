#pragma once

#include "real.h"

typedef struct {
    real x;
    real y;
    real z;
} vec3;

vec3 vec3_init();
vec3 vec3_init_values(real x, real y, real z);

vec3 vec3_add(const vec3 *v1, const vec3 *v2);
vec3 vec3_subtract(const vec3 *v1, const vec3 *v2);
vec3 vec3_multiply_scalar(const vec3 *v, real t);
vec3 vec3_multiply_vec3(const vec3 *v1, const vec3 *v2);

real vec3_length2(const vec3 *v);               //kvadratna duzina
real vec3_length(const vec3 *v);
vec3 vec3_normalized(const vec3 *v);            //normalizuje do jedinicnog vektora
real vec3_dot(const vec3 *v1, const vec3 *v2);

typedef struct {
    vec3 origin;
    vec3 direction;     //jedinicni vektor koji samo daje pravac i smer u prostoru
} ray;

ray ray_init(const vec3 *origin, const vec3 *direction);
