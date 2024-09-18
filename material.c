#include "material.h"

material material_init() {
    material m;
    m.color = vec3_init();          // inicijalizovano sa 3 nule -> crno
    m.reflectivity = 0;
    return m;
}

material material_init_with_values(vec3 c, real refl) {
    material m;
    m.color = c;
    m.reflectivity = refl;
    return m;
}

vec3 material_color_rgb(uint32_t rgb) {
    const float max_color = 255.0f;
    float r = ((rgb & 0xff0000) >> 16) / max_color;
    float g = ((rgb & 0x00ff00) >> 8) / max_color;
    float b = (rgb & 0x0000ff) / max_color;
    return vec3_init_values(real_float(b), real_float(g), real_float(r));
}

int material_light_mode;

vec3 material_color_sky[2];
vec3 material_color_horizon[2];
vec3 material_color_floor1[2];
vec3 material_color_floor2[2];
