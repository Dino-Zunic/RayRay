#include "scene.h"
#include "real.h"
#include "vec3.h"
#include "material.h"
#include <math.h>

#define FLOAT_INFINITY 0x75300000
#define EPSILON 0x00000147

const sphere *scene_get_spheres(const scene *sc) {
    return sc->spheres;
}

int scene_get_num_of_spheres(const scene *sc) {
    return sc->num_of_spheres;
}

rng *scene_get_rng(scene *sc) {
    return &sc->random_gen;
}

camera *scene_get_camera(scene *sc) {
    return &sc->cam;
}

real scene_clamp_real(real x) {             //ogranici x da bude izmedju 0 i 1
    if (x < 0) {
        return 0;
    }
    if (x > 0x00010000) {
        return 0x00010000;
    }
    return x;
}

vec3 scene_clamp_vec3(const vec3 *color) {
    vec3 result;
    result.x = scene_clamp_real(color->x);
    result.y = scene_clamp_real(color->y);
    result.z = scene_clamp_real(color->z);
    return result;
}

vec3 get_sky_color(const ray *r) {
    const int n = 2;
    const int offset = ((0x00010000 << n) - 0x00010000);
    const int m = 1;
    int y = scene_clamp_real(r->direction.y << m);
    real k = (y + offset) >> n;

    vec3 sky_color = material_color_sky[material_light_mode];
    vec3 horizon_color = material_color_horizon[material_light_mode];


    vec3 temp1 = vec3_multiply_scalar(&horizon_color, real_sub(0x00010000, k));
    vec3 temp2 = vec3_multiply_scalar(&sky_color, k);
    vec3 result_color = vec3_add(&temp1, &temp2);

    // Return the interpolated color
    return result_color;
}

/*
 * P(t) = origin.y + t*direction.y = 0 , jer ide do poda, y = 0 -> racunamo samo u y smeru
 * 0 = origin.y + t*direction.y
 * t = -origin.y/direction.y        , koliko daleko treba ici u smeru direction da bi y postalo 0, tj. da dodje do preseka
 */

int scene_intersect_plane(const ray *r, real *t) {              // da li zrak preseca "pod" - beskonacnu ravan na y = 0
    int intersects = (r->origin.y >> 31) ^ (r->direction.y >> 31);
    int parallel = r->direction.y == 0;
    if (!intersects || parallel) {
        return 0;
    }
    *t = -real_div(r->origin.y, r->direction.y);          // u t se upise udaljenost od pocetka zraka do tacke preseka
    return 1;
}

vec3 scene_get_floor_color(const vec3 *point, real t) {
    int x = int_real(point->x);       //koordinate na podu su x, z -> idu u celobrojne vrednosti
    int z = int_real(point->z);

    int is_white = (x + z) % 2 == 0;                    // kao a sahovsku tablu racuna da li je kvadrat beo ili crn
    vec3 base1 = material_color_floor1[material_light_mode];
    vec3 base2 = material_color_floor2[material_light_mode];
    vec3 mid = vec3_add(&base1, &base2);
    mid = vec3_multiply_scalar(&mid, real_float(0.5f));
    vec3 base = is_white ? base1 : base2;
    t = t / 100;
    t = real_mul(t, t);
    real k = real_div(real_int(1), real_int(1) + t);
    vec3 temp1 = vec3_multiply_scalar(&base, k);
    k = real_int(1) - k;
    vec3 temp2 = vec3_multiply_scalar(&mid, k);
    return vec3_add(&temp1, &temp2);
}

//recursive function
vec3 scene_trace_ray(scene *sc, const ray *r, int depth) {          //pracenje zraka
    if (depth >= MAX_DEPTH) {                                       //MAX_DEPTH - koliko puta pratimo odbijanje zraka
        return get_sky_color(r);
    }

    real t_closest = FLOAT_INFINITY;                                    //najbliza tacka preseka
    const sphere *closest_sphere = NULL;                                //najbliza sfera
    int hit_plane = 0;                                                  //da li je pogodio pod
    real t_plane = FLOAT_INFINITY;                                      //udaljenost do tacke preseka sa podom

    // provera preseka sa sferama -> podesi najblizu sa kojom se sece
    for (int i = 0; i < sc->num_of_spheres; ++i) {
        real t = 0;
        if (sphere_intersect(&sc->spheres[i], r, &t) && t < t_closest) {
            t_closest = t;
            closest_sphere = &sc->spheres[i];
        }
    }
    //provera preseka sa podom
    if (scene_intersect_plane(r, &t_plane) && !closest_sphere) {
        t_closest = t_plane;
        closest_sphere = NULL;
        hit_plane = 1;
    }

    vec3 temp0, temp1, temp2;

    if (closest_sphere || hit_plane) {
        t_closest -= EPSILON; // da bi tacka preseka sa objektom sigurno bila van objekta(lakse za kasnije odbijanje)

        temp0 = vec3_multiply_scalar(&r->direction, t_closest);
        
        //hit_point=origin + t_closest × direction
        vec3 hit_point = vec3_add(
            &r->origin,
            &temp0
        );
        material mat;
        vec3 reflected_direction;
        if (hit_plane) {
            //reflektivnost je 0.2f, a boja je boja poda u tacki preseka
            mat = material_init_with_values(scene_get_floor_color(&hit_point, t_closest), 0x00003333);
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
        vec3 reflected_color = scene_trace_ray(sc, &(ray){hit_point, reflected_direction}, depth + 1);
        temp0 = vec3_multiply_scalar(&mat.color, real_sub(0x00010000, mat.reflectivity));
        temp1 = vec3_multiply_scalar(&reflected_color, mat.reflectivity);
        temp2 = vec3_add(
            &temp0,
            &temp1
        );
        return scene_clamp_vec3(&temp2);
    }
    return get_sky_color(r);
}

void scene_reset(scene *sc) {
    camera_setup(scene_get_camera(sc));
    sc->num_of_spheres = 0;
    sc->marked = 0;
    sc->ghost_mode = 0;
    return;
    const real spread = 0x000a0000;

    for (int i = 0; i < MAX_SPHERES; ++i) {
        real rand = rng_rand(&sc->random_gen);
        // rand *= rand;
        // rand *= rand;
        real radius = real_add(real_mul(rng_rand(&sc->random_gen), 0x00020000), 0x00003333);
        vec3 pos = vec3_init_values(
            real_sub(real_mul(rng_rand(&sc->random_gen), spread << 1), spread),
            real_add(real_mul(rng_rand(&sc->random_gen), spread << 1), radius),
            real_sub(real_mul(rng_rand(&sc->random_gen), spread << 1), spread)
        );

        scene_add_sphere(sc, &pos, radius);
    }

}

scene globalScene;

scene *get_scene() {
    return &globalScene;
}

void scene_add_sphere(scene *sc, const vec3 *position, real radius) {
    if (sc->num_of_spheres < MAX_SPHERES) {
        if (position->y < radius) {
            return;
        }
        for (int i = 0; i < sc->num_of_spheres; i++) {
            vec3 pointer = vec3_subtract(position, &sc->spheres[i].center);
            real min_distance = radius + sc->spheres[i].radius;
            real min_distance2 = real_mul(min_distance, min_distance);
            if (vec3_length2(&pointer) < min_distance2) {
                return;
            }
        }
        sphere new_sphere;
        new_sphere.center = *position;
        new_sphere.radius = radius;

        vec3 colors[] = {
            material_color_rgb(0xee003f),
            material_color_rgb(0x00ee25),
            material_color_rgb(0x0052ee),
            material_color_rgb(0x4600D2)
        };

        int index = (int)(4 * float_real(rng_rand(&sc->random_gen))) % 4;
        vec3 color = colors[index];
        real reflectivity = rng_rand(&sc->random_gen);

        new_sphere.mat = material_init_with_values(color, reflectivity);
        sc->spheres[sc->num_of_spheres++] = new_sphere;
    }
}

void scene_remove_sphere(scene *sc, sphere *sp) {
    if (!sp) {
        return;
    }
    for (int i = 0; i < sc->num_of_spheres; i++) {
        if (&sc->spheres[i] == sp) {
            sc->num_of_spheres--;
            sc->spheres[i] = sc->spheres[sc->num_of_spheres];
            return;
        }
    }
}
