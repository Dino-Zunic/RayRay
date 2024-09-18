#include "real.h"

typedef int64_t wide_real;

real real_add(real x, real y) {
    return x + y;
}
real real_sub(real x, real y) {
    return x - y;
}
real real_mul(real x, real y) {
    wide_real product = (wide_real)x * (wide_real)y;
    return (real)(product >> 16);
}
real real_div(real x, real y) {
    if (y == 0) {                   //easier
        return 0;
    }
    wide_real q = ((wide_real)x << 16) / y;
    return (real)q;
}

real real_float(float x) {
    return x * 0x00010000;
}
real real_int(int x) {
    return x << 16;
}
float float_real(real x) {
    return 1.0f * x / 0x00010000;
}
int int_real(real x) {
    return x >> 16;
}

real real_sqrt(real x) {
    real result = x;
    for (int i = 0; i < 16; i++) {
        result = real_add(result, real_div(x, result)) >> 1;
    }
    return result;
}

real real_sqrt_inv(real x) {
    return real_div(real_int(1), real_sqrt(x)); // could be improved to not use division
}

static real real_project(real x) {
    real temp = (int64_t) x * 0x28be >> 32;
    return x - temp * REAL_PI_TWO;
}

static real real_cos_taylor(real x) {   //Tejlorov razvoj za prva 3 clana
    real result = 0x00010000;
    x = real_mul(x, x);
    real term = -x >> 1;
    result = real_add(result, term);

    real x2 = real_mul(x, x);
    term = (int64_t) x2 * 178956970 >> 32;
    result = real_add(result, term);

    real x3 = real_mul(x, x2);
    term = (int64_t) x3 * (-5965232) >> 32;
    result = real_add(result, term);

    return result;
}

real real_cos(real x) {         //pogadja kvadrant
    if (x < 0) {
        x = -x;
    }
    const int flip_sign = x >= REAL_PI_HALF && x < REAL_PI_THREE_HALFS;
    if (x > REAL_PI_HALF) {
        if (x <= REAL_PI) {
            x = REAL_PI - x;
        } else if (x <= REAL_PI_THREE_HALFS) {
            x = x - REAL_PI;
        } else {
            x = REAL_PI_TWO - x;
        }

    }
    /*if (x <= REAL_PI_HALF) {
        x = x;
    } else if (x <= REAL_PI) {
        x = REAL_PI - x;
    } else if (x <= REAL_PI_THREE_HALFS) {
        x = x - REAL_PI;
    } else {
        x = REAL_PI_TWO - x;
    }*/

    real result = real_cos_taylor(x);
    if (flip_sign) {
        result = -result;
    }
    return result;
}

// static real real_sin_taylor(real x) {
//     real x2 = real_mul(x, x);
//     real x3 = real_mul(x2, x);
//     real term1 = (int64_t) x3 * 715827882 >> 32;
//     term1 = ~term1;
//     real x5 = real_mul(x3, x2);
//     real term2 = (int64_t) x5 * 35791394 >> 32;
//     real x7 = real_mul(x5, x2);
//     real term3 = (int64_t) x7 * 851968 >> 32;
//     term3 = ~term3;
//
//     real result = x;
//     result = real_add(result, term1);
//     result = real_add(result, term2);
//     result = real_add(result, term3);
//
//     return result;
// }

real real_sin(real x) {
    return real_cos(REAL_PI_HALF - x);
    // int flip_sign = x >> 31;
    // if (x >> 31) {
    //     x = ~x;
    // }
    // x = real_project(x);
    // if (x > REAL_PI) {
    //     flip_sign = !flip_sign;
    // }
    // if (x > REAL_PI_HALF) {
    //     if (x <= REAL_PI) {
    //         x = REAL_PI - x;
    //     } else if (x <= REAL_PI_THREE_HALFS) {
    //         x = x - REAL_PI;
    //     } else {
    //         x = REAL_PI_TWO - x;
    //     }
    // }
    // real result = real_sin_taylor(x);
    // if (flip_sign) {
    //     result = ~result;
    // }
    // return result;
}

