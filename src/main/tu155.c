#include "common.h"
#include "shared.h"
#include "main/sef_164.h"
#include "main/tu155.h"


/* Converts a direction into pitch (angles->x) and yaw (angles->y). */
void vu0DirectionToAngles(Vector4 *angles, Vector4 *direction, int unusedArg2, int unusedArg3) {
    float negatedY;
    float horizontalLength;
    float pitch;
    float yaw;

    negatedY = -direction->y;
    __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(direction) : "memory");
    __asm__ __volatile__("vmul.xz vf1, vf1, vf1" : : : "memory");
    __asm__ __volatile__("vaddz.x vf1, vf1, vf1z" : : : "memory");
    __asm__ __volatile__("vsqrt Q, vf1x" : : : "memory");
    __asm__ __volatile__("sqc2 vf0, 0(%0)" : : "r"(angles) : "memory");
    __asm__ __volatile__("vwaitq" : : : "memory");
    __asm__ __volatile__("vaddq.x vf1, vf0, Q" : : : "memory");
    __asm__ __volatile__("qmfc2.ni $8, vf1\n\tmtc1 $8, %0" : "=f"(horizontalLength) : : "$8", "memory");
    pitch = srsAtan2(negatedY, horizontalLength);
    angles->x = pitch;
    yaw = srsAtan2(direction->x, direction->z);
    angles->y = yaw;
}

/* Global flag word; sv_get_image_item_161 also reads it with masks. */
extern u32 D_0069DAE0;

u32 func_00252E00(u32 mask) {
    return D_0069DAE0 & mask;
}

void func_00252E10(u32 mask) {
    D_0069DAE0 |= mask;
}

void func_00252E20(u32 mask) {
    D_0069DAE0 &= ~mask;
}

void func_00252E38(u32 mask) {
    D_0069DAE0 ^= mask;
}

INCLUDE_ASM("asm/main/nonmatchings/tu155", lightSetLogDump);
