#include "common.h"
#include "shared.h"

void *MMathNormalizeVector2(void *destination, const Vector4 *source)
{
    __asm__ __volatile__(
        "lqc2 vf1,0(%1)\n\t"
        "vmul.xyz vf2xyz,vf1xyz,vf1xyz\n\t"
        "vaddy.x vf2x,vf2x,vf2y\n\t"
        "vaddz.x vf2x,vf2x,vf2z\n\t"
        "vrsqrt Q,vf0w,vf2x\n\t"
        "vmove.w vf2w,vf0w\n\t"
        "vwaitq\n\t"
        "vmulq.xyz vf2xyz,vf1xyz,Q\n\t"
        "sqc2 vf2,0(%0)"
        : "+r"(destination)
        : "r"(source)
        : "memory"
    );
    return destination;
}

/* Vector length over x, y and z (VU0 square, sum, square root; the result is a float returned in $f0). */
INCLUDE_ASM("asm/main/nonmatchings/tu189", MMathCalcLength);

/* Vector length over x and y only. */
INCLUDE_ASM("asm/main/nonmatchings/tu189", MMathCalcLengthXY);

/* Vector length over x and z only. */
INCLUDE_ASM("asm/main/nonmatchings/tu189", MMathCalcLengthXZ);

/* Vector length over y and z only. */
INCLUDE_ASM("asm/main/nonmatchings/tu189", MMathCalcLengthYZ);
