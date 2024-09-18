#pragma once
#include <stdint.h>

#define REAL_PI 0x0003243f
#define REAL_PI_TWO 0x0006487f
#define REAL_PI_THREE_HALFS 0x0004b65f
#define REAL_PI_HALF 0x00019220

// abstarcion of floating point number on 32 bits, higher 16 are whole, and lower 16 are digits after coma
typedef int32_t real;

real real_add(real x, real y);
real real_sub(real x, real y);
real real_mul(real x, real y);
real real_div(real x, real y);

real real_float(float x);
real real_int(int x);
float float_real(real x);
int int_real(real x);

real real_sqrt(real x);
real real_sqrt_inv(real x);
real real_cos(real x);
real real_sin(real x);
