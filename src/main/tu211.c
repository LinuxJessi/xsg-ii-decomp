#include "common.h"
#include "main/tu213.h"
#include "main/xgl_2_028.h"

INCLUDE_ASM("asm/main/nonmatchings/tu211", quadDrawTransformed);

INCLUDE_ASM("asm/main/nonmatchings/tu211", quadDrawRepeated);

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_00278FC8);

#include "shared.h"
#include "vu0_macro.h"

/* Linear interpolation from `from` to `to` by fraction; the same body as animLerp in tu209. */
f32 func_00279180(f32 from, f32 to, f32 fraction) {
    return ((to - from) * fraction) + from;
}

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_00279190);


void recordSetAlphaFlags(int *record)
{
    int value;
    int extra;

    value = record[3];
    extra = record[2];
    if (record[1] != 0) {
        gsQueueSetAlphaFlags(value, extra, value);
        return;
    }
    gsQueueSetAlphaFlags(0, 0, value);
}

INCLUDE_ASM("asm/main/nonmatchings/tu211", quadDrawRotating);

extern f32 degToRad;
void xglMatrixUnit(Matrix4 matrix);
void xglMatrixRotZ(Matrix4 destination, Matrix4 source, f32 angle);
void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);

/* Rotates four vectors by angle (degrees) around the point (centerX, centerY). */
void quadRotateAroundCenter(Vector4 *vectors, f32 angle, f32 centerX, f32 centerY)
{
    AlignedVector4 center;
    AlignedVector4 offset;
    AlignedVector4 rotated;
    Matrix4 rotation;
    int i;

    center.x = centerX;
    center.y = centerY;
    center.z = 0.0f;
    xglMatrixUnit(rotation);
    xglMatrixRotZ(rotation, rotation, angle * degToRad);
    for (i = 0; i < 4; i++) {
        Vu0SubVector(&offset, vectors, &center);
        xglVectorMulMat(&rotated, rotation, &offset);
        __asm__ __volatile__("lqc2 vf20, 0(%0)" : : "r"(&rotated) : "memory");
        __asm__ __volatile__("lqc2 vf21, 0(%0)" : : "r"(&center) : "memory");
        __asm__ __volatile__("vadd.xyzw vf20, vf20, vf21" : : : "memory");
        __asm__ __volatile__("sqc2 vf20, 0(%0)" : : "r"(vectors) : "memory");
        vectors++;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_002796E0);

INCLUDE_ASM("asm/main/nonmatchings/tu211", randomPointInRect);

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_00279AB8);

extern int recordUpdatePaused;
extern int randomSeed;

/* Seeded random index below limit (or the game RNG when the seed flag is clear). */
int randomSeededIndex(f32 *values, int limit) {
    if (recordUpdatePaused == 0) {
        return xglSRand();
    }
    return (int) (((values[0] * 20.0f) / values[1]) + (f32) randomSeed) % limit;
}

void randomRefreshSeed(void) {
    if (recordUpdatePaused == 0) {
        randomSeed = xglSRand();
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_00279BD0);

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_00279EA8);

/* Wraps an angle in degrees into the range 0 to 360. */
f32 angleWrap360(f32 angle) {
    if (angle > 360.0f) {
        angle -= 360.0f;
    }
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_0027A130);

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_0027A1C8);

f32 func_0027A620(int sum, f32 angle);

int angleAdvance(int angle, int delta) {
    int sum;

    sum = angle + delta;
    return (int) func_0027A620(sum, (f32) sum);
}

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_0027A490);

f32 func_0027A620(int sum, f32 angle) {
    if (angle > 360.0f) {
        angle -= 360.0f;
    }
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

INCLUDE_ASM("asm/main/nonmatchings/tu211", func_0027A660);
