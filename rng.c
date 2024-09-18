#include "rng.h"

rng rng_init() {
    rng r;
    r.seed = 1;
    return r;
}

void rng_srand(rng *r, unsigned int s) {
    r->seed = s;
}

real rng_rand(rng *r) {
    // todo recreate
    r->seed = (214013 * r->seed + 2531011) & 0x7FFFFFFF;
    return real_float(r->seed / 2147483648.0f);
}
