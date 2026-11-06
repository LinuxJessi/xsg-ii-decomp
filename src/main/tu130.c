#include "common.h"

float func_002A1790(float ratio);

/*
 * Angle of the vector (x, y) in radians: atan of y / x, then moved into the
 * half-plane of the vector. A zero x is replaced by a tiny value.
 */
float xglAtan2Angle(float x, float y)
{
    float angle;
    float divisor;

    divisor = x;
    if (divisor == 0.0f) {
        divisor = 0.00001f;
    }
    angle = func_002A1790(y / divisor);
    if (divisor > 0.0f) {
        if (angle < 0.0f) {
            angle += 6.2831855f;
        }
    } else {
        angle += 3.1415927f;
    }
    return angle;
}

INCLUDE_ASM("asm/main/nonmatchings/tu130", func_0023ED20);

INCLUDE_ASM("asm/main/nonmatchings/tu130", func_0023ED60);
