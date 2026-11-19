#include "common.h"
#include "shared.h"
#include "main/tu190.h"

/* Distance from first to second (MMathCalcDist in X1). The original reserves a 16-byte frame
 * that no instruction touches: without the unused `difference` local (the vector VU0 keeps in
 * vf1) gcc emits no stack adjustment. */
float MMathCalcDist(const Vector4 *first, const Vector4 *second)
{
    Vector4 difference;
    float result;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 0(%2)\n\t"
        "vsub.xyz vf1, vf2, vf1\n\t"
        "vmul.xyz vf1, vf1, vf1\n\t"
        "vaddy.x vf1, vf1, vf1y\n\t"
        "vaddz.x vf1, vf1, vf1z\n\t"
        "vsqrt Q, vf1x\n\t"
        "vwaitq\n\t"
        "vaddq.x vf1, vf0, Q\n\t"
        "qmfc2.ni $8, vf1\n\t"
        "mtc1 $8, %0"
        : "=f"(result)
        : "r"(first), "r"(second)
        : "$8", "memory");
    return result;
}

/* Distance from first to second in the XZ plane (MMathCalcDistXZ in X1); the unused
 * `difference` local reproduces the 16-byte frame, as in MMathCalcDist. */
float MMathCalcDistXZ(const Vector4 *first, const Vector4 *second)
{
    Vector4 difference;
    float result;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 0(%2)\n\t"
        "vsub.xz vf1, vf2, vf1\n\t"
        "vmul.xz vf1, vf1, vf1\n\t"
        "vaddz.x vf1, vf1, vf1z\n\t"
        "vsqrt Q, vf1x\n\t"
        "vwaitq\n\t"
        "vaddq.x vf1, vf0, Q\n\t"
        "qmfc2.ni $8, vf1\n\t"
        "mtc1 $8, %0"
        : "=f"(result)
        : "r"(first), "r"(second)
        : "$8", "memory");
    return result;
}

void *MMathNormalizeVector2(void *destination, const Vector4 *source);

/* Unit vector from source towards target, stored at destination (MMathCalcDirVector in X1).
 * A plain `return MMathNormalizeVector2(...)` compiles to a sibling call (`j`); the original
 * calls it with `jal` and returns its result, which the do { } while (0) reproduces. */
Vector4 *MMathCalcDirVector(Vector4 *destination, const Vector4 *source, const Vector4 *target)
{
    Vector4 *difference = destination;
    Vector4 *normalized;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 0(%2)\n\t"
        "vsub.xyz vf2, vf2, vf1\n\t"
        "sqc2 vf2, 0(%0)"
        : "+r"(difference)
        : "r"(source), "r"(target)
        : "memory");
    do {
        normalized = MMathNormalizeVector2(destination, difference);
    } while (0);
    return normalized;
}
