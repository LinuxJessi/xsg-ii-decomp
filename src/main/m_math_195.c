#include "common.h"
#include "shared.h"

typedef float Matrix4[4][4];

typedef struct StudioCamera {
    u32 active;
    u32 state;
    u8 unmodeled_08[0x18];
    float nearClip;
    float farClip;
    u8 unmodeled_28[0x48];
    Vector4 screenOffset;
    Vector4 screenScale;
    u8 unmodeled_90[4];
    float fov;
    u8 unmodeled_98[8];
    DoublewordVector4 rotation;
    Vector4 angleVelocity;
    u8 unmodeled_c0[0x10];
    DoublewordVector4 position;
    u8 unmodeled_e0[0x40];
    DoublewordVector4 chasePosition;
    u8 unmodeled_130[0x40];
    Matrix4 cameraToWorldMatrix;
    Matrix4 occlusionTransform; /* 0x1B0: read by face_point.c's setup_occlusion/check_occlusion */
    u8 unmodeled_1f0[0x280];
    Matrix4 viewMatrix;
    u8 unmodeled_4b0[0x40];
    Matrix4 clipMatrices[2]; /* 0x4F0: the pair _ModelCalcClipInit loads into vf12..vf19 */
    u8 unmodeled_570[0x80];
} StudioCamera;

Vector4 *MMathApplyMatrix(Vector4 *out, Matrix4 *matrix, const Vector4 *vector);
Matrix4 *MMathRotateMatrixYX(Matrix4 *out, const Matrix4 *matrix, const Vector4 *angles);
void MMathCalcAngle(Vector4 *out, const Vector4 *source, const Vector4 *target);

/* out = matrix with rows 0..2 scaled by scale; row 3, the translation row, is copied unchanged. */
Matrix4 *MMathScaleMatrixRotS(Matrix4 *out, const Matrix4 *matrix, float scale)
{
    __asm__ __volatile__(
        "mfc1 $8, %2\n\t"
        "qmtc2.ni $8, vf5\n\t"
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 16(%1)\n\t"
        "lqc2 vf3, 32(%1)\n\t"
        "lqc2 vf4, 48(%1)\n\t"
        "vmulx.xyzw vf1, vf1, vf5x\n\t"
        "vmulx.xyzw vf2, vf2, vf5x\n\t"
        "vmulx.xyzw vf3, vf3, vf5x\n\t"
        "sqc2 vf1, 0(%0)\n\t"
        "sqc2 vf2, 16(%0)\n\t"
        "sqc2 vf3, 32(%0)\n\t"
        "sqc2 vf4, 48(%0)"
        : : "r"(out), "r"(matrix), "f"(scale) : "$8", "memory");
    return out;
}

/* out = matrix scaled by scale in every row; the w of row 3 is kept. */
Matrix4 *MMathScaleMatrixS(Matrix4 *out, const Matrix4 *matrix, float scale)
{
    __asm__ __volatile__(
        "mfc1 $8, %2\n\t"
        "qmtc2.ni $8, vf5\n\t"
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 16(%1)\n\t"
        "lqc2 vf3, 32(%1)\n\t"
        "lqc2 vf4, 48(%1)\n\t"
        "vmulx.xyzw vf1, vf1, vf5x\n\t"
        "vmulx.xyzw vf2, vf2, vf5x\n\t"
        "vmulx.xyzw vf3, vf3, vf5x\n\t"
        "vmulx.xyz vf4, vf4, vf5x\n\t"
        "sqc2 vf1, 0(%0)\n\t"
        "sqc2 vf2, 16(%0)\n\t"
        "sqc2 vf3, 32(%0)\n\t"
        "sqc2 vf4, 48(%0)"
        : : "r"(out), "r"(matrix), "f"(scale) : "$8", "memory");
    return out;
}

Matrix4 *MMathTranslateMatrix(Matrix4 *out, Matrix4 *matrix, const Vector4 *translation)
{
    __asm__ __volatile__(
        "lqc2 $vf5,0(%2)\n\t"
        "lqc2 $vf1,0(%1)\n\t"
        "lqc2 $vf2,16(%1)\n\t"
        "lqc2 $vf3,32(%1)\n\t"
        "lqc2 $vf4,48(%1)\n\t"
        "vmulax.xyzw ACCxyzw,$vf1xyzw,$vf5x\n\t"
        "vmadday.xyzw ACCxyzw,$vf2xyzw,$vf5y\n\t"
        "vmaddaz.xyzw ACCxyzw,$vf3xyzw,$vf5z\n\t"
        "vmaddw.xyzw $vf4xyzw,$vf4xyzw,$vf0w\n\t"
        "sqc2 $vf1,0(%0)\n\t"
        "sqc2 $vf2,16(%0)\n\t"
        "sqc2 $vf3,32(%0)\n\t"
        "sqc2 $vf4,48(%0)"
        : : "r"(out), "r"(matrix), "r"(translation) : "memory");
    return out;
}

/* MMathCalcLookAtMatrix (MMathCalcLookAtMatrix): pitch and yaw from source to target, the float argument as roll
 * (z of the angles), YXZ rotation over the identity; a non-null position argument goes to row 3. */
INCLUDE_ASM("asm/main/nonmatchings/m_math_195", MMathCalcLookAtMatrix);

/* out = matrix with its 3x3 block transposed; the fourth column is kept. */
Matrix4 *MMathTransposeMatrix3(Matrix4 *out, const Matrix4 *matrix)
{
    __asm__ __volatile__(
        "lqc2 vf6, 0(%1)\n\t"
        "lqc2 vf7, 16(%1)\n\t"
        "lqc2 vf8, 32(%1)\n\t"
        "lqc2 vf4, 48(%1)\n\t"
        "vmove.xw vf1, vf6\n\t"
        "vaddy.x vf2, vf0, vf6y\n\t"
        "vaddz.x vf3, vf0, vf6z\n\t"
        "vaddx.y vf1, vf0, vf7x\n\t"
        "vmove.yw vf2, vf7\n\t"
        "vaddz.y vf3, vf0, vf7z\n\t"
        "vaddx.z vf1, vf0, vf8x\n\t"
        "vaddy.z vf2, vf0, vf8y\n\t"
        "vmove.zw vf3, vf8\n\t"
        "sqc2 vf1, 0(%0)\n\t"
        "sqc2 vf2, 16(%0)\n\t"
        "sqc2 vf3, 32(%0)\n\t"
        "sqc2 vf4, 48(%0)"
        : : "r"(out), "r"(matrix) : "memory");
    return out;
}

/* out = the full 4x4 transpose of matrix. */
Matrix4 *MMathTransposeMatrix(Matrix4 *out, const Matrix4 *matrix)
{
    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 16(%1)\n\t"
        "lqc2 vf3, 32(%1)\n\t"
        "lqc2 vf4, 48(%1)\n\t"
        "vsub.xyzw vf5, vf0, vf0\n\t"
        "vaddx.x vf6, vf0, vf1x\n\t"
        "vaddy.x vf7, vf0, vf1y\n\t"
        "vaddz.x vf8, vf0, vf1z\n\t"
        "vaddw.x vf9, vf0, vf1w\n\t"
        "vaddx.y vf6, vf0, vf2x\n\t"
        "vaddy.y vf7, vf0, vf2y\n\t"
        "vaddz.y vf8, vf0, vf2z\n\t"
        "vaddw.y vf9, vf0, vf2w\n\t"
        "vaddx.z vf6, vf0, vf3x\n\t"
        "vaddy.z vf7, vf0, vf3y\n\t"
        "vaddz.z vf8, vf0, vf3z\n\t"
        "vaddw.z vf9, vf0, vf3w\n\t"
        "vaddx.w vf6, vf5, vf4x\n\t"
        "vaddy.w vf7, vf5, vf4y\n\t"
        "vaddz.w vf8, vf5, vf4z\n\t"
        "vaddw.w vf9, vf5, vf4w\n\t"
        "sqc2 vf6, 0(%0)\n\t"
        "sqc2 vf7, 16(%0)\n\t"
        "sqc2 vf8, 32(%0)\n\t"
        "sqc2 vf9, 48(%0)"
        : : "r"(out), "r"(matrix) : "memory");
    return out;
}

/* out = rotation (YX order) that turns the default direction toward target. */
Matrix4 *MMathCalcMatrixVector(Matrix4 *out, const Vector4 *target)
{
    Vector4 angles;

    MMathCalcAngle(&angles, 0, target);
    MMathRotateMatrixYX(out, 0, &angles);
    return out;
}

/* out = (0, 0, 1, 1) times matrix: row 2 plus row 3, the transformed +Z point. */
Vector4 *MMathCalcVectorMatrixZ(Vector4 *out, Matrix4 *matrix)
{
    __asm__ __volatile__(
        "vmove.xyw vf1, vf0\n\t"
        "vaddw.z vf1, vf0, vf0w\n\t"
        "sqc2 vf1, 0(%0)"
        : : "r"(out) : "memory");
    MMathApplyMatrix(out, matrix, out);
    return out;
}

/* out = angles from the default direction to the transformed +Z point of matrix. */
Vector4 *MMathCalcAngleMatrixZ(Vector4 *out, Matrix4 *matrix)
{
    Vector4 point;

    MMathCalcVectorMatrixZ(&point, matrix);
    MMathCalcAngle(out, 0, &point);
    return out;
}

void MMathRotTransPers(Vector4 *destination, const StudioCamera *camera, const Matrix4 *matrix, const Vector4 *point)
{
    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "lqc2 vf3, 16(%1)\n\t"
        "lqc2 vf4, 32(%1)\n\t"
        "lqc2 vf5, 48(%1)\n\t"
        "vmulax.xyzw ACC, vf2, vf1x\n\t"
        "vmadday.xyzw ACC, vf3, vf1y\n\t"
        "vmaddaz.xyzw ACC, vf4, vf1z\n\t"
        "vmaddw.xyzw vf1, vf5, vf1w\n\t"
        "vdiv Q, vf0w, vf1w\n\t"
        "lqc2 vf2, 0(%2)\n\t"
        "lqc2 vf3, 0(%3)\n\t"
        "vsub.xyzw vf4, vf0, vf0\n\t"
        "vwaitq\n\t"
        "vmulq.xyz vf1, vf1, Q\n\t"
        "vmula.xyz ACC, vf1, vf2\n\t"
        "vmaddw.xyz vf4, vf3, vf0w\n\t"
        "vftoi4.xy vf1, vf4\n\t"
        "vftoi0.zw vf1, vf4\n\t"
        "sqc2 vf1, 0(%4)"
        : : "r"(point), "r"(matrix), "r"(&camera->screenScale), "r"(&camera->screenOffset), "r"(destination) : "memory");
}
