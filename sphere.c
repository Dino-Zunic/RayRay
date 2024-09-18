#include "sphere.h"
#include "real.h"
#include "vec3.h"

sphere sphere_init() {
    sphere s;
    s.center = vec3_init();
    s.radius = 0;
    s.mat = material_init();
    return s;
}

sphere sphere_init_with_values(vec3 c, real r, material m) {
    sphere s;
    s.center = c;
    s.radius = r;
    s.mat = m;
    return s;
}

int sphere_intersect(const sphere *s, const ray *r, real *t) {      //ako zrak preseca, duzinu do tacke preseka upise u t i vrati 1
    const real EPSILON = 0x00000147;

    //solving quadratic equation bellow
    vec3 oc = vec3_subtract(&r->origin, &s->center);        //vektor od centra sfere do pocetka zraka          oc = (O - C)
    real b = vec3_dot(&oc, &r->direction);
    real c = real_sub(vec3_dot(&oc, &oc), real_mul(s->radius, s->radius));
    real discriminant = real_sub(real_mul(b, b), c);

    if (0 < discriminant) {
        real sqrt_discriminant = real_sqrt(discriminant);
        real temp = real_sub(-b, sqrt_discriminant);
        if (EPSILON < temp) {
            *t = temp;
            return 1;
        }
        temp = real_add(-b, sqrt_discriminant);
        if (EPSILON < temp) {               //da li je presek dovoljn blizu 0, da bi bio validan, ako je suvise blizu, moze se zanemariti
            *t = temp;
            return 1;
        }
    }
    return 0;
}

/*
 * zrak: pocetna tacka: O
 *       normalizovan vektor pravca: D
 *       jednacina: P(t) =
 * sfera: tacka na povrsini sfere: P
 *        centar: C
 *        poluprecnik: r
 *        jednacina: (P-C)*(P-C) = r^2
 *        zamenom:   (O + t*D - C)*(O + t*D - C) = r^2
 *                   t^2 * (D*D) +2t(D*(O - C)) + (O - C)*(O - C) - r^2 = 0             oc = O-C, b = (D*(O - C)), c = (O - C)*(O - C) - r^2
 */