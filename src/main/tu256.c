#include "common.h"
#include "shared.h"
#include "main/tu256.h"

typedef float Matrix4[4][4];

f32 cosf(f32);
f32 sinf(f32);
void xglCurMatrixMul(Matrix4 *matrix);
extern Matrix4 xglMulRotMatrixX;
extern Matrix4 xglMulRotMatrixY;
extern Matrix4 xglMulRotMatrixZ;
extern Matrix4 xglMulTranslateMatrix;
extern Matrix4 xglMulScaleMatrix;
extern Matrix4 xglMulScaleXYZMatrix;

/* Rotation about X: cosine on the [1][1] and [2][2] diagonal, sine at [1][2], -sine at [2][1]. */
void xglCurMulRotateX(f32 angle)
{
    f32 cosine;
    f32 sine;

    cosine = cosf(angle);
    sine = sinf(angle);
    xglMulRotMatrixX[2][2] = cosine;
    xglMulRotMatrixX[1][1] = cosine;
    xglMulRotMatrixX[1][2] = sine;
    xglMulRotMatrixX[2][1] = -sine;
    xglCurMatrixMul(xglMulRotMatrixX);
}

/* Rotation about Y: cosine on the [0][0] and [2][2] diagonal, sine at [2][0], -sine at [0][2]. */
void xglCurMulRotateY(f32 angle)
{
    f32 cosine;
    f32 sine;

    cosine = cosf(angle);
    sine = sinf(angle);
    xglMulRotMatrixY[2][2] = cosine;
    xglMulRotMatrixY[0][0] = cosine;
    xglMulRotMatrixY[2][0] = sine;
    xglMulRotMatrixY[0][2] = -sine;
    xglCurMatrixMul(xglMulRotMatrixY);
}

/* Rotation about Z: cosine on the [0][0] and [1][1] diagonal, sine at [0][1], -sine at [1][0]. */
void xglCurMulRotateZ(f32 angle)
{
    f32 cosine;
    f32 sine;

    cosine = cosf(angle);
    sine = sinf(angle);
    xglMulRotMatrixZ[1][1] = cosine;
    xglMulRotMatrixZ[0][0] = cosine;
    xglMulRotMatrixZ[0][1] = sine;
    xglMulRotMatrixZ[1][0] = -sine;
    xglCurMatrixMul(xglMulRotMatrixZ);
}

/* Current matrix = translation matrix * current matrix; translation fills row 3 of xglMulTranslateMatrix. */
void xglCurMulTranslate(const Vector4 *translation)
{
    xglMulTranslateMatrix[3][0] = translation->x;
    xglMulTranslateMatrix[3][1] = translation->y;
    xglMulTranslateMatrix[3][2] = translation->z;
    xglCurMatrixMul(xglMulTranslateMatrix);
}

/* Current matrix = uniform scale matrix (xglMulScaleMatrix) * current matrix. */
void xglCurMulScale(f32 scale)
{
    xglMulScaleMatrix[0][0] = scale;
    xglMulScaleMatrix[1][1] = scale;
    xglMulScaleMatrix[2][2] = scale;
    xglCurMatrixMul(xglMulScaleMatrix);
}

/* Current matrix = per-axis scale matrix (xglMulScaleXYZMatrix) * current matrix. */
void xglCurMulScaleXYZ(const Vector4 *scale)
{
    xglMulScaleXYZMatrix[0][0] = scale->x;
    xglMulScaleXYZMatrix[1][1] = scale->y;
    xglMulScaleXYZMatrix[2][2] = scale->z;
    xglCurMatrixMul(xglMulScaleXYZMatrix);
}

/* Writes 1/x, 1/y, 1/z on the diagonal of xglMulInvScaleMatrix for each non-zero component (zero components are skipped), then multiplies the current matrix by it. */
INCLUDE_ASM("asm/main/nonmatchings/tu256", xglCurMulInvScaleXYZ);
