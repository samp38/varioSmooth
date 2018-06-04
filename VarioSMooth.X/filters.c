#include "pic24_all.h"



// ceofficients for speed flter (maxflat Wn = 0.01)
#define B1_0 73
#define A1_0 1000
#define A1_1 854


// feedforward coefficients
#define B_0_s    4 // shift 4 = *16
#define B_1_s  	 5 // shift 5 = *32
#define B_2_s  	 4 // shift 4 = *16
// feedback coefficients,
#define A_0_sr 	11 // shift 11 right = div by 2048
#define A_1  	3553
#define A_2  	1569

#define INTEGER_FACTOR 11       // filter Value will be multiplied by 2ÎNTEGER_FACTOR for precision

s64 x_1 = 0LL;
s64 x_2 = 0LL;
s64 y_1 = 0LL;
s64 y_2 = 0LL;

s32 filterPressure(s32 x) {
    s64 xBIG = (s64)(x) << INTEGER_FACTOR;
    s64 y = (xBIG<<B_0_s) + (x_1<<B_1_s) + (x_2<<B_2_s) + (((s64)A_1*y_1) - ((s64)A_2*y_2));

    x_2 = x_1;
    x_1 = xBIG;

    y_2 = y_1;
    y_1 = y>>A_0_sr;

    return (s32)(y_1 >> INTEGER_FACTOR);
}


s32 y1_1 = 0L;
s32 x1_1 = 0L;

// 1st order IIR low pass for speed
s32 speedIIRFilter(s32 x) {
    s32 y = (B1_0 * (x + x1_1)) + (A1_1 * y1_1);
    x1_1 = x;
    y1_1 = y/(A1_0);
    return y1_1;
}
