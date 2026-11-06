#include "common.h"
#include "shared.h"
#include "vu0_macro.h"

/* Sets the VU0 current matrix (vf27-vf30) from a 4x4 matrix. */
void xglCurMatrixSet(f32 matrix[4][4]) {
    Vu0SetCurrentMatrix(matrix);
}

/* Current matrix = matrix * current matrix: the four rows of matrix are multiplied by vf27-vf30 and the product is kept in vf27-vf30. */
void xglCurMatrixMul(f32 matrix[4][4]) {
    __asm__ __volatile__(
        "lqc2 vf2, 0(%0)\n\t"
        "lqc2 vf3, 16(%0)\n\t"
        "lqc2 vf4, 32(%0)\n\t"
        "lqc2 vf5, 48(%0)\n\t"
        "vmulax.xyzw ACC, vf27, vf2x\n\t"
        "vmadday.xyzw ACC, vf28, vf2y\n\t"
        "vmaddaz.xyzw ACC, vf29, vf2z\n\t"
        "vmaddw.xyzw vf2, vf30, vf2w\n\t"
        "vmulax.xyzw ACC, vf27, vf3x\n\t"
        "vmadday.xyzw ACC, vf28, vf3y\n\t"
        "vmaddaz.xyzw ACC, vf29, vf3z\n\t"
        "vmaddw.xyzw vf3, vf30, vf3w\n\t"
        "vmulax.xyzw ACC, vf27, vf4x\n\t"
        "vmadday.xyzw ACC, vf28, vf4y\n\t"
        "vmaddaz.xyzw ACC, vf29, vf4z\n\t"
        "vmaddw.xyzw vf4, vf30, vf4w\n\t"
        "vmulax.xyzw ACC, vf27, vf5x\n\t"
        "vmadday.xyzw ACC, vf28, vf5y\n\t"
        "vmaddaz.xyzw ACC, vf29, vf5z\n\t"
        "vmaddw.xyzw vf5, vf30, vf5w\n\t"
        "vmove.xyzw vf27, vf2\n\t"
        "vmove.xyzw vf28, vf3\n\t"
        "vmove.xyzw vf29, vf4\n\t"
        "vmove.xyzw vf30, vf5"
        :
        : "r"(matrix)
        : "memory"
    );
}

/* Stores the VU0 current matrix (vf27-vf30) into matrix. */
void xglCurMatrixGet(f32 matrix[4][4]) {
    Vu0GetCurrentMatrix(matrix);
}

/* destination = current matrix * (vector.xyz, 1); the w component is passed through. */
void xglCurApplyVector(Vector4 *destination, const Vector4 *vector) {
    __asm__ __volatile__(
        "lqc2 vf2, 0(%1)\n\t"
        "vmulax.xyzw ACC, vf27, vf2x\n\t"
        "vmadday.xyzw ACC, vf28, vf2y\n\t"
        "vmaddaz.xyzw ACC, vf29, vf2z\n\t"
        "vmaddw.xyzw vf2, vf30, vf2w\n\t"
        "sqc2 vf2, 0(%0)"
        :
        : "r"(destination), "r"(vector)
        : "memory"
    );
}

/* Empty body: no statements, no role shown. */
void func_00292FB8(void) {
}

/* Builds an X-axis rotation in xglRotMatrixX (cos at [1][1] and [2][2], sin at [1][2], -sin at [2][1]) and makes it the current matrix. */
INCLUDE_ASM("asm/main/nonmatchings/tu255", xglCurSetRotateX);

f32 cosf(f32);
f32 sinf(f32);
extern f32 xglRotMatrixY[4][4];

/* Builds a Y-axis rotation in xglRotMatrixY (cos at [0][0] and [2][2], sin at [2][0], -sin at [0][2]) and makes it the current matrix. */
void xglCurSetRotateY(f32 angle) {
    f32 cosine = cosf(angle);
    f32 sine = sinf(angle);

    xglRotMatrixY[0][0] = cosine;
    xglRotMatrixY[2][0] = sine;
    xglRotMatrixY[0][2] = -sine;
    xglRotMatrixY[2][2] = cosine;
    xglCurMatrixSet(xglRotMatrixY);
}

extern f32 xglRotMatrixZ[4][4];

/* Builds a Z-axis rotation in xglRotMatrixZ (cos at [0][0] and [1][1], sin at [0][1], -sin at [1][0]) and makes it the current matrix. */
void xglCurSetRotateZ(f32 angle) {
    f32 cosine = cosf(angle);
    f32 sine = sinf(angle);

    xglRotMatrixZ[0][0] = cosine;
    xglRotMatrixZ[0][1] = sine;
    xglRotMatrixZ[1][0] = -sine;
    xglRotMatrixZ[1][1] = cosine;
    xglCurMatrixSet(xglRotMatrixZ);
}

extern f32 xglTranslateMatrix[4][4];

/* Translation matrix in xglTranslateMatrix (x, y, z in row 3), made the current matrix. */
void xglCurSetTranslate(const Vector4 *vector) {
    xglTranslateMatrix[3][0] = vector->x;
    xglTranslateMatrix[3][1] = vector->y;
    xglTranslateMatrix[3][2] = vector->z;
    xglCurMatrixSet(xglTranslateMatrix);
}

extern f32 xglScaleMatrix[4][4];

/* Uniform scale matrix in xglScaleMatrix (scale on the diagonal), made the current matrix. */
void xglCurSetScale(f32 scale) {
    xglScaleMatrix[0][0] = scale;
    xglScaleMatrix[1][1] = scale;
    xglScaleMatrix[2][2] = scale;
    xglCurMatrixSet(xglScaleMatrix);
}

extern f32 xglScaleXYZMatrix[4][4];

/* Per-axis scale matrix in xglScaleXYZMatrix (scale.x, scale.y, scale.z on the diagonal), made the current matrix. */
void xglCurSetScaleXYZ(const Vector4 *scale) {
    xglScaleXYZMatrix[0][0] = scale->x;
    xglScaleXYZMatrix[1][1] = scale->y;
    xglScaleXYZMatrix[2][2] = scale->z;
    xglCurMatrixSet(xglScaleXYZMatrix);
}
