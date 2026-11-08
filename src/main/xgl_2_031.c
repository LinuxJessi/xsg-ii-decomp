#include "common.h"
#include "shared.h"
#include "main/xgl_2_031.h"

extern void xglVectorLength(float *destination, const Vector4 *vector);
extern void xglMatrixStackSave(float matrix[4][4]);
void xglVectorScale(Vector4 *destination, const Vector4 *source, float scale);

typedef unsigned int XglQuadword __attribute__((mode(TI)));

typedef union Matrix {
    float elements[16];
    Matrix4 rows;
    XglQuadword quadwords[4];
} Matrix;

void xglVectorMulMat(Vector4 *destination, Matrix4 matrix,
                     const Vector4 *vector);
void xglVectorScaleXYZ(Vector4 *destination, const Vector4 *source, float scale);
void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);
void xglMatrixStackLoad(float matrix[4][4]);
void xglVectorMulAdd(Vector4 *destination, const Vector4 *left,
                     const Vector4 *right, const Vector4 *addend);
extern unsigned long long iRandSeed;
void xglRandSeedInit(void);
void xglVectorDiv(Vector4 *destination, const Vector4 *numerator,
                  const Vector4 *divisor);
void xglVectorDivXYZ(Vector4 *destination, const Vector4 *numerator,
                     const Vector4 *divisor);
void xglVectorMulAddXYZ(Vector4 *destination, const Vector4 *left,
                        const Vector4 *right, const Vector4 *addend);
void xglVectorMulSub(Vector4 *destination, const Vector4 *left,
                     const Vector4 *right, const Vector4 *subtrahend);
void xglVectorMulSubXYZ(Vector4 *destination, const Vector4 *left,
                        const Vector4 *right, const Vector4 *subtrahend);
void xglVectorDivAdd(Vector4 *destination, const Vector4 *numerator,
                     const Vector4 *divisor, const Vector4 *addend);
void xglVectorDivAddXYZ(Vector4 *destination, const Vector4 *numerator,
                        const Vector4 *divisor, const Vector4 *addend);
void xglVectorDivSub(Vector4 *destination, const Vector4 *numerator,
                     const Vector4 *divisor, const Vector4 *subtrahend);
void xglVectorDivSubXYZ(Vector4 *destination, const Vector4 *numerator,
                        const Vector4 *divisor, const Vector4 *subtrahend);
extern void xglMatrixMul(Matrix *destination, Matrix *left, Matrix *right);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMemCopy16b);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMemCopy8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMemCopy8b);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMemCopy4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMemCopy4b);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMemCopy2);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMemCopy2b);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglSin);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglCos);

void xglVectorDiv(Vector4 *destination, const Vector4 *numerator,
                  const Vector4 *divisor)
{
    destination->x = numerator->x / divisor->x;
    destination->y = numerator->y / divisor->y;
    destination->z = numerator->z / divisor->z;
    destination->w = numerator->w / divisor->w;
}

void xglVectorDivXYZ(Vector4 *destination, const Vector4 *numerator,
                     const Vector4 *divisor)
{
    destination->x = numerator->x / divisor->x;
    destination->y = numerator->y / divisor->y;
    destination->z = numerator->z / divisor->z;
    destination->w = numerator->w;
}

void xglVectorMulAdd(Vector4 *destination, const Vector4 *left,
                     const Vector4 *right, const Vector4 *addend)
{
    destination->x = left->x * right->x + addend->x;
    destination->y = left->y * right->y + addend->y;
    destination->z = left->z * right->z + addend->z;
    destination->w = left->w * right->w + addend->w;
}

void xglVectorMulAddXYZ(Vector4 *destination, const Vector4 *left,
                        const Vector4 *right, const Vector4 *addend)
{
    destination->x = left->x * right->x + addend->x;
    destination->y = left->y * right->y + addend->y;
    destination->z = left->z * right->z + addend->z;
    destination->w = left->w;
}

void xglVectorMulSub(Vector4 *destination, const Vector4 *left,
                     const Vector4 *right, const Vector4 *subtrahend)
{
    destination->x = left->x * right->x - subtrahend->x;
    destination->y = left->y * right->y - subtrahend->y;
    destination->z = left->z * right->z - subtrahend->z;
    destination->w = left->w * right->w - subtrahend->w;
}

void xglVectorMulSubXYZ(Vector4 *destination, const Vector4 *left,
                        const Vector4 *right, const Vector4 *subtrahend)
{
    destination->x = left->x * right->x - subtrahend->x;
    destination->y = left->y * right->y - subtrahend->y;
    destination->z = left->z * right->z - subtrahend->z;
    destination->w = left->w;
}

void xglVectorDivAdd(Vector4 *destination, const Vector4 *numerator,
                     const Vector4 *divisor, const Vector4 *addend)
{
    destination->x = numerator->x / divisor->x + addend->x;
    destination->y = numerator->y / divisor->y + addend->y;
    destination->z = numerator->z / divisor->z + addend->z;
    destination->w = numerator->w / divisor->w + addend->w;
}

void xglVectorDivAddXYZ(Vector4 *destination, const Vector4 *numerator,
                        const Vector4 *divisor, const Vector4 *addend)
{
    destination->x = numerator->x / divisor->x + addend->x;
    destination->y = numerator->y / divisor->y + addend->y;
    destination->z = numerator->z / divisor->z + addend->z;
    destination->w = numerator->w;
}

void xglVectorDivSub(Vector4 *destination, const Vector4 *numerator,
                     const Vector4 *divisor, const Vector4 *subtrahend)
{
    destination->x = numerator->x / divisor->x - subtrahend->x;
    destination->y = numerator->y / divisor->y - subtrahend->y;
    destination->z = numerator->z / divisor->z - subtrahend->z;
    destination->w = numerator->w / divisor->w - subtrahend->w;
}

void xglVectorDivSubXYZ(Vector4 *destination, const Vector4 *numerator,
                        const Vector4 *divisor, const Vector4 *subtrahend)
{
    destination->x = numerator->x / divisor->x - subtrahend->x;
    destination->y = numerator->y / divisor->y - subtrahend->y;
    destination->z = numerator->z / divisor->z - subtrahend->z;
    destination->w = numerator->w;
}

void xglVectorScale(Vector4 *destination, const Vector4 *source, float scale)
{
    destination->x = source->x * scale;
    destination->y = source->y * scale;
    destination->z = source->z * scale;
    destination->w = source->w * scale;
}

void xglVectorScaleXYZ(Vector4 *destination, const Vector4 *source, float scale)
{
    destination->x = source->x * scale;
    destination->y = source->y * scale;
    destination->z = source->z * scale;
    destination->w = source->w;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorScaleAdd);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorScaleAddXYZ);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorScaleSub);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorScaleSubXYZ);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorInter);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorInterXYZ);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorInner);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorInner4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorOuter);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorOuterNormal);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglPlaneOuter);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglPlaneNormal);

void xglVectorLength(float *destination, const Vector4 *vector)
{
    __asm__ __volatile__(
        "lqc2 vf3, 0(%1)\n"
        "vmul.xyz vf3, vf3, vf3\n"
        "vaddy.x vf3, vf3, vf3y\n"
        "vaddz.x vf3, vf3, vf3z\n"
        "vsqrt Q, vf3x\n"
        "vwaitq\n"
        "vaddq.x vf4, vf0, Q\n"
        "qmfc2 $2, vf4\n"
        "sw $2, 0(%0)\n"
        :
        : "r"(destination), "r"(vector)
        : "$2", "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglPointLength);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglPointLengthXZ);

void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source)
{
    __asm__ __volatile__(
        "lqc2 vf2, 0(%1)\n\t"
        "vmul.xyz vf3xyz, vf2xyz, vf2xyz\n\t"
        "vaddy.x vf3x, vf3x, vf3y\n\t"
        "vaddz.x vf3x, vf3x, vf3z\n\t"
        "vrsqrt Q, vf0w, vf3x\n\t"
        "vwaitq\n\t"
        "vmulq.xyz vf2xyz, vf2xyz, Q\n\t"
        "sqc2 vf2, 0(%0)"
        :
        : "r"(destination), "r"(source)
        : "memory"
    );
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorClamp);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorClampXYZ);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglPlaneParameter);

void xglVectorMulMat(Vector4 *destination, Matrix4 matrix,
                     const Vector4 *vector)
{
    __asm__ __volatile__(
        "lqc2 vf1, 0(%2)\n\t"
        "lqc2 vf27, 0(%1)\n\t"
        "lqc2 vf28, 16(%1)\n\t"
        "lqc2 vf29, 32(%1)\n\t"
        "lqc2 vf30, 48(%1)\n\t"
        "vmulax.xyzw ACCxyzw, vf27xyzw, vf1x\n\t"
        "vmadday.xyzw ACCxyzw, vf28xyzw, vf1y\n\t"
        "vmaddaz.xyzw ACCxyzw, vf29xyzw, vf1z\n\t"
        "vmaddw.xyzw vf31xyzw, vf30xyzw, vf1w\n\t"
        "sqc2 vf31, 0(%0)"
        :
        : "r"(destination), "r"(matrix), "r"(vector)
        : "memory"
    );
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A979C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9858);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixUnit);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixUnit4s);

void xglMatrixMul(Matrix *destination, Matrix *left, Matrix *right)
{
    __asm__ __volatile__(
        "lqc2 vf2, 0(%2)\n\t"
        "lqc2 vf3, 16(%2)\n\t"
        "lqc2 vf4, 32(%2)\n\t"
        "lqc2 vf5, 48(%2)\n\t"
        "lqc2 vf27, 0(%1)\n\t"
        "lqc2 vf28, 16(%1)\n\t"
        "lqc2 vf29, 32(%1)\n\t"
        "lqc2 vf30, 48(%1)\n\t"
        "vmulax.xyzw ACCxyzw, vf27xyzw, vf2x\n\t"
        "vmadday.xyzw ACCxyzw, vf28xyzw, vf2y\n\t"
        "vmaddaz.xyzw ACCxyzw, vf29xyzw, vf2z\n\t"
        "vmaddw.xyzw vf2xyzw, vf30xyzw, vf2w\n\t"
        "vmulax.xyzw ACCxyzw, vf27xyzw, vf3x\n\t"
        "vmadday.xyzw ACCxyzw, vf28xyzw, vf3y\n\t"
        "vmaddaz.xyzw ACCxyzw, vf29xyzw, vf3z\n\t"
        "vmaddw.xyzw vf3xyzw, vf30xyzw, vf3w\n\t"
        "vmulax.xyzw ACCxyzw, vf27xyzw, vf4x\n\t"
        "vmadday.xyzw ACCxyzw, vf28xyzw, vf4y\n\t"
        "vmaddaz.xyzw ACCxyzw, vf29xyzw, vf4z\n\t"
        "vmaddw.xyzw vf4xyzw, vf30xyzw, vf4w\n\t"
        "vmulax.xyzw ACCxyzw, vf27xyzw, vf5x\n\t"
        "vmadday.xyzw ACCxyzw, vf28xyzw, vf5y\n\t"
        "vmaddaz.xyzw ACCxyzw, vf29xyzw, vf5z\n\t"
        "vmaddw.xyzw vf30xyzw, vf30xyzw, vf5w\n\t"
        "sqc2 vf2, 0(%0)\n\t"
        "sqc2 vf3, 16(%0)\n\t"
        "sqc2 vf4, 32(%0)\n\t"
        "sqc2 vf30, 48(%0)"
        :
        : "r"(destination), "r"(left), "r"(right)
        : "memory"
    );
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixReverse);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9B50);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9F88);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA0CC);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixRotV);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixRotX);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixRotY);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixRotZ);

void xglMatrixFrustum(Matrix *destination, Matrix *source,
                      float left, float right, float bottom, float top,
                      float nearPlane, float farPlane)
{
    Matrix projection;

    projection.elements[0] = (2.0f * nearPlane) / (right - left);
    projection.elements[1] = 0.0f;
    projection.elements[2] = 0.0f;
    projection.elements[3] = 0.0f;
    projection.elements[4] = 0.0f;
    projection.elements[5] = (2.0f * nearPlane) / (top - bottom);
    projection.elements[6] = 0.0f;
    projection.elements[7] = 0.0f;
    projection.elements[8] = (right + left) / (right - left);
    projection.elements[9] = (top + bottom) / (top - bottom);
    projection.elements[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    projection.elements[11] = -1.0f;
    projection.elements[12] = 0.0f;
    projection.elements[13] = 0.0f;
    projection.elements[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    projection.elements[15] = 0.0f;
    xglMatrixMul(destination, source, &projection);
}

#include "vu0_macro.h"

extern char D_108[];

void xglMatrixStackUnit(void)
{
    Vu0CallMicro((u32)&D_108 >> 3);
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixStackMul);

extern char D_1B8[];

void xglMatrixStackReverse(void)
{
    Vu0CallMicro((u32)&D_1B8 >> 3);
}

extern char D_218[];

void xglMatrixStackInverse(void)
{
    Vu0CallMicro((u32)&D_218 >> 3);
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA8B0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA8F4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixStackRotV);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixStackRotX);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixStackRotY);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixStackRotZ);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AAA60);

void xglMatrixStackSave(float matrix[4][4])
{
    __asm__ __volatile__(
        "ctc2.i %0,$vi0\n\t"
        "sqc2 $vf28,0(%0)\n\t"
        "sqc2 $vf29,16(%0)\n\t"
        "sqc2 $vf30,32(%0)\n\t"
        "sqc2 $vf31,48(%0)"
        :
        : "r"(matrix)
        : "memory"
    );
}

void xglMatrixStackLoad(float matrix[4][4])
{
    __asm__ __volatile__(
        "ctc2.i %0,$vi0\n\t"
        "lqc2 $vf28,0(%0)\n\t"
        "lqc2 $vf29,16(%0)\n\t"
        "lqc2 $vf30,32(%0)\n\t"
        "lqc2 $vf31,48(%0)"
        :
        : "r"(matrix)
        : "memory"
    );
}

extern char D_538[];

void xglMatrixStackPushUnit(void)
{
    Vu0CallMicro((u32)&D_538 >> 3);
}

extern char D_580[];

void xglMatrixStackPush(void)
{
    Vu0CallMicro((u32)&D_580 >> 3);
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixStackPop);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixStackMulVector);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixStackRTPS);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AAD20);

void xglQuaternion2Matrix(Matrix4 destination, const Vector4 *quaternion)
{
    __asm__ __volatile__(
        "lqc2 vf31, 0(%1)\n\t"
        "vaddw.x vf27x, vf0x, vf0w\n\t"
        "vaddw.y vf28y, vf0y, vf0w\n\t"
        "vaddw.z vf29z, vf0z, vf0w\n\t"
        "vadd.xyzw vf14xyzw, vf31xyzw, vf31xyzw\n\t"
        "vsub.w vf27w, vf27w, vf27w\n\t"
        "vsub.w vf28w, vf28w, vf28w\n\t"
        "vsub.w vf29w, vf29w, vf29w\n\t"
        "vmulx.xyzw vf15xyzw, vf31xyzw, vf14x\n\t"
        "vmuly.xyzw vf16xyzw, vf31xyzw, vf14y\n\t"
        "vmulz.xyzw vf17xyzw, vf31xyzw, vf14z\n\t"
        "vnop\n\t"
        "vnop\n\t"
        "vaddy.x vf20x, vf15x, vf16y\n\t"
        "vaddz.y vf20y, vf16y, vf17z\n\t"
        "vaddx.z vf20z, vf17z, vf15x\n\t"
        "vnop\n\t"
        "vsubw.z vf27z, vf15z, vf16w\n\t"
        "vaddw.y vf27y, vf15y, vf17w\n\t"
        "vsuby.x vf27x, vf27x, vf20y\n\t"
        "vsubw.x vf28x, vf16x, vf17w\n\t"
        "vaddw.z vf28z, vf16z, vf15w\n\t"
        "vsubz.y vf28y, vf28y, vf20z\n\t"
        "vsubw.y vf29y, vf17y, vf15w\n\t"
        "vaddw.x vf29x, vf17x, vf16w\n\t"
        "vsubx.z vf29z, vf29z, vf20x\n\t"
        "sqc2 vf0, 48(%0)\n\t"
        "sqc2 vf27, 0(%0)\n\t"
        "sqc2 vf28, 16(%0)\n\t"
        "sqc2 vf29, 32(%0)"
        :
        : "r"(destination), "r"(quaternion)
        : "memory"
    );
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglQuaternionInterpolateLinear);

void xglRandSeedInit(void)
{
    iRandSeed = 0x12345678ULL;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB2F0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglMatrixCopy);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorSubXYZ);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB3C0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB3F8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB72C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB7B0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB948);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ABAA8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ABCAC);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ABEF8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AC1A8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AC304);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AC4DC);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AC95C);

extern int xglMotionCacheValid;
extern Vector4 xglMotionCache[];

void *xglMotionCacheGet(void)
{
    if (xglMotionCacheValid != 0) {
        return xglMotionCache;
    }
    return 0;
}

void xglMotionCacheInvalidate(void)
{
    xglMotionCacheValid = 0;
}

/* One animated element of a motion pose (0x50 bytes per element). */
typedef struct XglMotionKey {
    Vector4 position;       /* 0x00 */
    Vector4 rotation;       /* 0x10: x, y, z angles */
    Vector4 target;         /* 0x20 */
    Vector4 direction;      /* 0x30 */
    float unk40;
    float unk44;
    float scale;            /* 0x48 */
    float unk4C;
} XglMotionKey;

typedef struct XglMotionEntry {
    int unk00;
    int type;               /* 0x04 */
} XglMotionEntry;

typedef struct XglMotion {
    u8 unk00[0x30];
    int entryCount;         /* 0x30 */
    u8 unk34[8];
    int keyOffset;          /* 0x3C */
    u8 unk40[0xB0];
    int entryOffsets[1];    /* 0xF0 */
} XglMotion;

/* A vector kept as a plain float[4] (like sceVu0FVECTOR); its w is element 3. Vector4 here changes the
 * address arithmetic of the w stores, so the casts to Vector4 * at the calls stay. */
typedef float XglFVector[4];

/* Result of xglMotionBuildPose, cleared with xglZeroQuadwords18 (0x18 quadwords). */
typedef struct XglMotionPose {
    XglFVector rows[6];     /* 0x000 */
    Vector4 viewDirection;  /* 0x060 */
    XglFVector axes[6];     /* 0x070 */
    Vector4 velocity;       /* 0x0D0 */
    Vector4 acceleration;   /* 0x0E0 */
    Vector4 forward;        /* 0x0F0 */
    Vector4 unk100[2];
    XglFVector points[3];   /* 0x120 */
    XglFVector pointDirections[3]; /* 0x150 */
} XglMotionPose;

void xglZeroQuadwords18(void *destination);
void xglVectorCopy(Vector4 *destination, const Vector4 *source);
void xglVectorAddXYZ(Vector4 *destination, const Vector4 *left, const Vector4 *right);
void xglVectorSubXYZ_001AF3F0(Vector4 *destination, const Vector4 *left, const Vector4 *right);
void xglMatrixStackRotX(float angle);
void xglMatrixStackRotY(float angle);
void xglMatrixStackRotZ(float angle);
void xglMatrixStackMulVector(Vector4 *destination, const Vector4 *source);
void func_001BB3FC(void *cache);
void func_001ABEF8(Vector4 *work, Vector4 *vector, Vector4 *direction);
int func_001AC1A8(Vector4 *result, XglMotionKey *key, Vector4 *origin);
int func_001AC304(Vector4 *result, Vector4 *direction, XglMotionKey *key, Vector4 *origin);
int func_001AC4DC(Vector4 *result, Vector4 *direction, XglMotionKey *key, Vector4 *origin);
int func_001AC95C(Vector4 *result, Vector4 *direction, XglMotionKey *key, Vector4 *origin);

/* Loads the key's Z, Y, X rotation into the matrix stack (identity first, zero angles skipped).
 * A plain block, not do/while (0): the loop's labels would renumber the output. */
#define LOAD_KEY_ROTATION(key) \
    { \
        xglMatrixStackUnit(); \
        if ((key)->rotation.z != 0.0f) { \
            xglMatrixStackRotZ((key)->rotation.z); \
        } \
        if ((key)->rotation.y != 0.0f) { \
            xglMatrixStackRotY((key)->rotation.y); \
        } \
        if ((key)->rotation.x != 0.0f) { \
            xglMatrixStackRotX((key)->rotation.x); \
        } \
    }

#define XGL_MOTION_TYPE_COUNT 19

void xglMotionBuildPose(XglMotion *motion, XglMotionPose *pose, Vector4 *origin)
{
    int i;
    int *offsets;
    Vector4 poseRows[12];

    offsets = motion->entryOffsets;
    memset(poseRows, 0, sizeof(poseRows));
    xglZeroQuadwords18(pose);
    for (i = 0; i < motion->entryCount; i++) {
        XglMotionEntry *entry = (XglMotionEntry *)((char *)motion + offsets[i]);

        if (entry->type < XGL_MOTION_TYPE_COUNT) {
            XglMotionKey *key = &((XglMotionKey *)((char *)motion + motion->keyOffset))[i];
            DoublewordAlignedVector4 dir = { 0.0f, 0.0f, -1.0f, 1.0f };
            Vector4 forceA;
            Vector4 forceB;
            Vector4 originB;
            Vector4 forceC;
            Vector4 originC;
            Vector4 forceD;
            Vector4 originD;
            Vector4 scaled;

            switch (entry->type) {
            case 9:
                xglVectorScale(xglMotionCache, &key->direction, key->scale);
                func_001BB3FC(xglMotionCache);
                xglMotionCacheValid = 1;
                break;
            case 1:
                xglVectorScale(&pose->velocity, &key->direction, key->scale);
                xglVectorScale(&pose->acceleration, &key->direction, key->scale);
                break;
            case 2:
                xglVectorScale(&pose->velocity, &key->direction, key->scale);
                xglVectorScale(&pose->acceleration, &key->target, key->scale);
                LOAD_KEY_ROTATION(key)
                                xglMatrixStackMulVector(&dir, &dir);
                xglVectorCopy(&pose->viewDirection, &dir);
                break;
            case 3:
                if (!func_001AC1A8(&forceA, key, origin)) {
                    break;
                }
                xglVectorScale(&forceA, &forceA, key->scale);
                xglVectorAddXYZ(&pose->velocity, &pose->velocity, &forceA);
                xglVectorAddXYZ(&pose->acceleration, &pose->acceleration, &forceA);
                break;
            case 7:
                if (!func_001AC95C(&forceB, &originB, key, origin)) {
                    break;
                }
                xglVectorScale(&forceB, &forceB, key->scale);
                xglVectorAddXYZ(&pose->velocity, &pose->velocity, &forceB);
                xglVectorAddXYZ(&pose->acceleration, &pose->acceleration, &forceB);
                break;
            case 6:
                if (!func_001AC304(&forceC, &originC, key, origin)) {
                    break;
                }
                xglVectorScale(&forceC, &forceC, key->scale);
                xglVectorAddXYZ(&pose->velocity, &pose->velocity, &forceC);
                xglVectorAddXYZ(&pose->acceleration, &pose->acceleration, &forceC);
                break;
            case 4:
                if (!func_001AC4DC(&forceD, &originD, key, origin)) {
                    break;
                }
                xglVectorScale(&forceD, &forceD, key->scale);
                xglVectorAddXYZ(&pose->velocity, &pose->velocity, &forceD);
                xglVectorAddXYZ(&pose->acceleration, &pose->acceleration, &forceD);
                break;
            case 8:
                dir.z = 1.0f;
                LOAD_KEY_ROTATION(key)
                                xglMatrixStackMulVector(&dir, &dir);
                xglVectorScale(&scaled, &key->direction, key->scale);
                func_001ABEF8(poseRows, &scaled, &dir);
                break;
            case 10:
                dir.z = 1.0f;
                LOAD_KEY_ROTATION(key)
                                xglMatrixStackMulVector(&pose->forward, &dir);
                pose->forward.w = 1.0f;
                break;
            case 11:
                xglVectorSubXYZ_001AF3F0(&pose->forward, &key->position, origin);
                xglVectorNormal_001A9200(&pose->forward, &pose->forward);
                pose->forward.w = 1.0f;
                break;
            }
        } else {
            break;
        }
    }
    {
        int count;

        count = 0;
        for (i = 0; i < motion->entryCount; i++) {
            XglMotionEntry *entry = (XglMotionEntry *)((char *)motion + offsets[i]);

            if (entry->type >= XGL_MOTION_TYPE_COUNT) {
                XglMotionKey *key = &((XglMotionKey *)((char *)motion + motion->keyOffset))[i];
                Vector4 forceA;
                Vector4 originA;
                Vector4 forceB;
                Vector4 originB;
                Vector4 forceC;
                Vector4 originC;
                Vector4 forceD;
                Vector4 originD;

                switch (entry->type) {
                case 19:
                    if (count < 3) {
                        xglVectorCopy((Vector4 *)pose->points[count], &key->position);
                        xglVectorScale((Vector4 *)pose->pointDirections[count], &key->direction, key->scale * 128.0f);
                        pose->points[count][3] = 1.0f;
                        count++;
                    }
                    break;
                case 20:
                case 35:
                    if (!func_001AC1A8(&forceA, key, origin)) {
                        break;
                    }
                    xglVectorSubXYZ_001AF3F0(&originA, &key->position, origin);
                    xglVectorScale(&forceA, &forceA, key->scale);
                    func_001ABEF8(poseRows, &forceA, &originA);
                    break;
                case 24:
                    if (!func_001AC95C(&forceB, &originB, key, origin)) {
                        break;
                    }
                    xglVectorScale(&forceB, &forceB, key->scale);
                    func_001ABEF8(poseRows, &forceB, &originB);
                    break;
                case 21:
                    if (!func_001AC4DC(&forceC, &originC, key, origin)) {
                        break;
                    }
                    xglVectorScale(&forceC, &forceC, key->scale);
                    func_001ABEF8(poseRows, &forceC, &originC);
                    break;
                case 23:
                    if (!func_001AC304(&forceD, &originD, key, origin)) {
                        break;
                    }
                    xglVectorScale(&forceD, &forceD, key->scale);
                    func_001ABEF8(poseRows, &forceD, &originD);
                    break;
                }
            }
        }
    }
    {
        int k;

        for (k = 0; k < 6; k++) {
            xglVectorCopy((Vector4 *)pose->axes[k], &poseRows[k]);
            xglVectorCopy((Vector4 *)pose->rows[k], &poseRows[k + 6]);
            pose->axes[k][3] = 0;
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ADBC4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ADCB0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ADD34);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ADDD4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ADFCC);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ADFF0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AE370);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AE43C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AE5E4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AE6B8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AEEE0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AF0F0);
