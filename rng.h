#pragma once
#include "real.h"

//random number generator

typedef struct {
    unsigned int seed;
} rng;

rng rng_init();
void rng_srand(rng *r, unsigned int s);
real rng_rand(rng *r);
