#pragma once

#include "vec3.h"
#include "material.h"

typedef struct {
    vec3 center;
    real radius;
    material mat;
} sphere;

sphere sphere_init();
sphere sphere_init_with_values(vec3 c, real r, material m);

int sphere_intersect(const sphere *s, const ray *r, real *t);
