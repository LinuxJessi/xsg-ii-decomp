#include "common.h"
#include "shared.h"

typedef float Matrix4[4][4];

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A7DC8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A7E1C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A7E70);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A7EC4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A7F18);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A7F6C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A7FC0);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A89B0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A8A70);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A8B1C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A8BDC);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A8C88);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A8D78);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorInner);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A8EE0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", xglVectorOuter);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A8FBC);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9020);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9080);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A914C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A91A8);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9250);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A94A0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9678);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A994C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A998C);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9A74);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9B50);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001A9F88);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA0CC);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA260);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA454);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA4F0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA58C);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA7B0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA7EC);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA838);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA874);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA8B0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA8F4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AA938);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AABA8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AABE4);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AAC20);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AAC64);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AACB8);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB1DC);

void xglRandSeedInit(void)
{
    iRandSeed = 0x12345678ULL;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB2F0);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB344);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001AB388);

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ACF5C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ACF9C);

INCLUDE_ASM("asm/main/nonmatchings/xgl_2_031", func_001ACFC0);

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
