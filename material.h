#pragma once     //in VS

#include "vec3.h"

typedef struct {
    vec3 color;
    real reflectivity;  // 1.0 for a perfect mirror, 0.0 for a diffuse surface
} material;

material material_init();
material material_init_with_values(vec3 c, real refl);
vec3 material_color_rgb(uint32_t rgb);

extern int material_light_mode;

extern vec3 material_color_sky[2];
extern vec3 material_color_horizon[2];
extern vec3 material_color_floor1[2];
extern vec3 material_color_floor2[2];
