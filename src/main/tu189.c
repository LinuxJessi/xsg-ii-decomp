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

/* Vector length over x, y and z (VU0 square, sum, square root; the result is a float returned in $f0).
 * The length is written to lane x of a quadword local: that local is the original's unused 16-byte frame. */
float MMathCalcLength(const Vector4 *vector)
{
    Vector4 length;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "vmul.xyz vf1, vf1, vf1\n\t"
        "vaddy.x vf1, vf1, vf1y\n\t"
        "vaddz.x vf1, vf1, vf1z\n\t"
        "vsqrt Q, vf1x\n\t"
        "vwaitq\n\t"
        "vaddq.x vf1, vf0, Q\n\t"
        "qmfc2.ni $8, vf1\n\t"
        "mtc1 $8, %0"
        : "=f"(length.x) : "r"(vector) : "$8", "memory");
    return length.x;
}

/* Vector length over x and y only. */
float MMathCalcLengthXY(const Vector4 *vector)
{
    Vector4 length;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "vmul.xy vf1, vf1, vf1\n\t"
        "vaddy.x vf1, vf1, vf1y\n\t"
        "vsqrt Q, vf1x\n\t"
        "vwaitq\n\t"
        "vaddq.x vf1, vf0, Q\n\t"
        "qmfc2.ni $8, vf1\n\t"
        "mtc1 $8, %0"
        : "=f"(length.x) : "r"(vector) : "$8", "memory");
    return length.x;
}

/* Vector length over x and z only. */
float MMathCalcLengthXZ(const Vector4 *vector)
{
    Vector4 length;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "vmul.xz vf1, vf1, vf1\n\t"
        "vaddz.x vf1, vf1, vf1z\n\t"
        "vsqrt Q, vf1x\n\t"
        "vwaitq\n\t"
        "vaddq.x vf1, vf0, Q\n\t"
        "qmfc2.ni $8, vf1\n\t"
        "mtc1 $8, %0"
        : "=f"(length.x) : "r"(vector) : "$8", "memory");
    return length.x;
}

/* Vector length over y and z only. */
float MMathCalcLengthYZ(const Vector4 *vector)
{
    Vector4 length;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "vmul.yz vf1, vf1, vf1\n\t"
        "vaddz.y vf1, vf1, vf1z\n\t"
        "vsqrt Q, vf1y\n\t"
        "vwaitq\n\t"
        "vaddq.x vf1, vf0, Q\n\t"
        "qmfc2.ni $8, vf1\n\t"
        "mtc1 $8, %0"
        : "=f"(length.x) : "r"(vector) : "$8", "memory");
    return length.x;
}
