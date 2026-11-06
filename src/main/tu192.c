#include "common.h"
#include "shared.h"

/* MMathVectorDotProduct (MMathVectorDotProduct): dot product of the xyz lanes, returned in $f0. */
INCLUDE_ASM("asm/main/nonmatchings/tu192", MMathVectorDotProduct);

Vector4 *MMathVectorCrossProduct(Vector4 *destination, const Vector4 *first, const Vector4 *second)
{
    __asm__ __volatile__(
        "lqc2 vf1, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "vopmula.xyz ACC, vf1, vf2\n\t"
        "vopmsub.xyz vf1, vf2, vf1\n\t"
        "vmove.w vf1, vf0\n\t"
        "sqc2 vf1, 0(%2)"
        : : "r"(first), "r"(second), "r"(destination) : "memory");
    return destination;
}

Vector4 *MMathVectorInterpolation(Vector4 *destination, const Vector4 *first, const Vector4 *second, float parameter)
{
    __asm__ __volatile__(
        "mfc1 $8, %0\n\t"
        "qmtc2.ni $8, vf3\n\t"
        "vsubx.w vf3, vf0, vf3x\n\t"
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 0(%2)\n\t"
        "vmulaw.xyz ACC, vf1, vf3w\n\t"
        "vmaddx.xyz vf1, vf2, vf3x\n\t"
        "sqc2 vf1, 0(%3)"
        : : "f"(parameter), "r"(first), "r"(second), "r"(destination) : "$8", "memory");
    return destination;
}
