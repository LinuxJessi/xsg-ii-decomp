#include "common.h"
#include "shared.h"
#include "main/sef_164.h"


/* Heading in the XZ plane from first to second (MMathCalcDir in X1). The srsAtan2 result is
 * left in $f0 by the last call; the declared void return does not change that. */
void MMathCalcDir(const Vector4 *first, const Vector4 *second)
{
    const Vector4 **origin;

    origin = &first;
    srsAtan2(second->x - (*origin)->x, second->z - (*origin)->z);
}

/* MMathCalcAngle (MMathCalcAngle): pitch to out.x, yaw to out.y; null vectors default to MMathUnitVector. */
INCLUDE_ASM("asm/main/nonmatchings/m_math_191", MMathCalcAngle);

Vector4 *MMathCalcOffset(Vector4 *destination, const Vector4 *first, const Vector4 *second)
{
    __asm__ __volatile__(
        "lqc2 vf5, 0(%0)\n\t"
        "lqc2 vf6, 0(%1)\n\t"
        "vaddy.w vf7, vf5, vf6y\n\t"
        "vaddw.x vf4, vf0, vf7w\n\t"
        "vcallms 0xE8\n\t"
        "vaddx.z vf7, vf0, vf1x\n\t"
        "vaddw.x vf4, vf0, vf7w\n\t"
        "vcallms 0x20\n\t"
        "vmove.x vf7, vf1\n\t"
        "vmove.x vf4, vf6\n\t"
        "vcallms 0x20\n\t"
        "vaddx.y vf7, vf0, vf1x\n\t"
        "vmove.x vf4, vf6\n\t"
        "vcallms 0xE8\n\t"
        "vmulx.xz vf7, vf7, vf1x\n\t"
        "vmulaw.xyz ACC, vf7, vf6w\n\t"
        "vmaddw.xyz vf5, vf5, vf0w\n\t"
        "sqc2 vf5, 0(%2)"
        : : "r"(first), "r"(second), "r"(destination) : "memory");
    return destination;
}

Vector4 *MMathCalcOffsetXYZ(Vector4 *destination, const Vector4 *first, const Vector4 *second)
{
    __asm__ __volatile__(
        "lqc2 vf5, 0(%0)\n\t"
        "lqc2 vf6, 0(%1)\n\t"
        "vaddw.x vf4, vf0, vf5w\n\t"
        "vcallms 0xE8\n\t"
        "vmove.x vf7, vf1\n\t"
        "vaddw.x vf4, vf0, vf5w\n\t"
        "vcallms 0x20\n\t"
        "vaddz.x vf2, vf0, vf6z\n\t"
        "vsubx.z vf2, vf0, vf6x\n\t"
        "vmulax.xz ACC, vf6, vf7x\n\t"
        "vmaddx.xz vf6, vf2, vf1x\n\t"
        "vadd.xyz vf5, vf5, vf6\n\t"
        "sqc2 vf5, 0(%2)"
        : : "r"(first), "r"(second), "r"(destination) : "memory");
    return destination;
}

Vector4 *MMathSubVectorMulS(Vector4 *destination, const Vector4 *first, const Vector4 *second, float scale)
{
    __asm__ __volatile__(
        "mfc1 $8, %0\n\t"
        "qmtc2.ni $8, vf3\n\t"
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 0(%2)\n\t"
        "vsub.xyz vf1, vf1, vf2\n\t"
        "vmulx.xyz vf1, vf1, vf3x\n\t"
        "sqc2 vf1, 0(%3)"
        : : "f"(scale), "r"(first), "r"(second), "r"(destination) : "$8", "memory");
    return destination;
}

Vector4 *MMathSubVectorDivS(Vector4 *destination, const Vector4 *first, const Vector4 *second, float scale)
{
    __asm__ __volatile__(
        "mfc1 $8, %0\n\t"
        "qmtc2.ni $8, vf3\n\t"
        "lqc2 vf1, 0(%1)\n\t"
        "vdiv Q, vf0w, vf3x\n\t"
        "lqc2 vf2, 0(%2)\n\t"
        "vsub.xyz vf1, vf1, vf2\n\t"
        "vwaitq\n\t"
        "vmulq.xyz vf1, vf1, Q\n\t"
        "sqc2 vf1, 0(%3)"
        : : "f"(scale), "r"(first), "r"(second), "r"(destination) : "$8", "memory");
    return destination;
}

Vector4 *MMathDivVector(Vector4 *destination, const Vector4 *first, const Vector4 *second)
{
    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "vdiv Q, vf1x, vf2x\n\t"
        "vmove.w vf3, vf1\n\t"
        "vwaitq\n\t"
        "vaddq.x vf3, vf0, Q\n\t"
        "vdiv Q, vf1y, vf2y\n\t"
        "vwaitq\n\t"
        "vaddq.y vf3, vf0, Q\n\t"
        "vdiv Q, vf1z, vf2z\n\t"
        "vwaitq\n\t"
        "vaddq.z vf3, vf0, Q\n\t"
        "sqc2 vf3, 0(%2)"
        : : "r"(first), "r"(second), "r"(destination) : "memory");
    return destination;
}

Vector4 *MMathDivVectorS(Vector4 *destination, const Vector4 *source, float scale)
{
    __asm__ __volatile__(
        "mfc1 $8, %0\n\t"
        "qmtc2.ni $8, vf2\n\t"
        "lqc2 vf1, 0(%1)\n\t"
        "vdiv Q, vf0w, vf2x\n\t"
        "vwaitq\n\t"
        "vmulq.xyz vf1, vf1, Q\n\t"
        "sqc2 vf1, 0(%2)"
        : : "f"(scale), "r"(source), "r"(destination) : "$8", "memory");
    return destination;
}

Vector4 *MMathDivVector4(Vector4 *destination, const Vector4 *first, const Vector4 *second)
{
    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "vdiv Q, vf1x, vf2x\n\t"
        "vsub.w vf3, vf3, vf3\n\t"
        "vwaitq\n\t"
        "vaddq.x vf3, vf0, Q\n\t"
        "vdiv Q, vf1y, vf2y\n\t"
        "vwaitq\n\t"
        "vaddq.y vf3, vf0, Q\n\t"
        "vdiv Q, vf1z, vf2z\n\t"
        "vwaitq\n\t"
        "vaddq.z vf3, vf0, Q\n\t"
        "vdiv Q, vf1w, vf2w\n\t"
        "vwaitq\n\t"
        "vaddq.w vf3, vf3, Q\n\t"
        "sqc2 vf3, 0(%2)"
        : : "r"(first), "r"(second), "r"(destination) : "memory");
    return destination;
}

Vector4 *MMathDivVectorS4(Vector4 *destination, const Vector4 *source, float scale)
{
    __asm__ __volatile__(
        "mfc1 $8, %0\n\t"
        "qmtc2.ni $8, vf2\n\t"
        "lqc2 vf1, 0(%1)\n\t"
        "vdiv Q, vf0w, vf2x\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw vf1, vf1, Q\n\t"
        "sqc2 vf1, 0(%2)"
        : : "f"(scale), "r"(source), "r"(destination) : "$8", "memory");
    return destination;
}

/* MMathIsVectorEqual (MMathIsVectorEqual) and MMathIsVectorEqual4 (MMathIsVectorEqual4): 128-bit equality tests
 * returning a short 1 when the vectors are equal; they differ only in the permute before the second movn. */
INCLUDE_ASM("asm/main/nonmatchings/m_math_191", MMathIsVectorEqual);

INCLUDE_ASM("asm/main/nonmatchings/m_math_191", MMathIsVectorEqual4);
