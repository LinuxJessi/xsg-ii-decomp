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

INCLUDE_ASM("asm/main/nonmatchings/tu189", func_00271F28);

INCLUDE_ASM("asm/main/nonmatchings/tu189", func_00271F58);

INCLUDE_ASM("asm/main/nonmatchings/tu189", func_00271F88);

INCLUDE_ASM("asm/main/nonmatchings/tu189", func_00271FB8);
