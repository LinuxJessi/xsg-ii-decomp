#include "common.h"
#include "shared.h"
#include "main/sdv_033.h"

/*
 * Transforms point by matrix, projects it with the camera's screen scale and offset, and stores
 * the fixed-point screen position in destination with the clip mark in w: 0x8000 when the VU0 clip
 * flags are set, else 0. Returns that mark. The ori/cfc2/nop/movn lines are the library's clip-flag
 * read-out. The original keeps an untouched 16-byte frame: the unused scratch quadword.
 */
int MMathRotTransPersClip(Vector4 *destination, const XglCamera *camera, const Matrix4 *matrix, const Vector4 *point)
{
    Vector4 scratch;
    int clipped;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 0(%2)\n\t"
        "lqc2 vf3, 16(%2)\n\t"
        "lqc2 vf4, 32(%2)\n\t"
        "lqc2 vf5, 48(%2)\n\t"
        "vmulax.xyzw ACC, vf2, vf1x\n\t"
        "vmadday.xyzw ACC, vf3, vf1y\n\t"
        "vmaddaz.xyzw ACC, vf4, vf1z\n\t"
        "vmaddw.xyzw vf1, vf5, vf1w\n\t"
        "vdiv Q, vf0w, vf1w\n\t"
        "ctc2.ni $0, $vi18\n\t"
        "lqc2 vf2, 0(%3)\n\t"
        "lqc2 vf3, 0(%4)\n\t"
        "ori $9, $0, 0x8000\n\t"
        "vwaitq\n\t"
        "vmulq.xyz vf4, vf1, Q\n\t"
        "vclipw.xyz vf4xyz, vf0w\n\t"
        "vmula.xyz ACC, vf4, vf2\n\t"
        "vmaddw.xyz vf5, vf3, vf0w\n\t"
        "vftoi4.xy vf1, vf5\n\t"
        "vftoi0.z vf1, vf5\n\t"
        "cfc2.ni %0, $vi18\n\t"
        "nop\n\t"
        "movn %0, $9, %0\n\t"
        "qmtc2.ni %0, vf2\n\t"
        "vmr32.w vf1, vf2\n\t"
        "sqc2 vf1, 0(%5)"
        : "=r"(clipped)
        : "r"(point), "r"(matrix), "r"(&camera->screen.screenScale), "r"(&camera->screen.screenOffset), "r"(destination)
        : "$9", "memory");
    return clipped;
}

/*
 * Shortest-arc rotation quaternion from the unit vector from to the unit vector to:
 * s = sqrt(2 (1 + from.to)), xyz = (from x to) / s, w = s / 2. Nearly opposite vectors
 * (|s| < 0.01) give (0, 1, 0, 0) and return 0; otherwise returns 1.
 */
int MMathQuatBetweenVectors(Vector4 *destination, const Vector4 *from, const Vector4 *to)
{
    Vector4 arc;

    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "lqc2 vf2, 0(%2)\n\t"
        "vmul.xyz vf3, vf1, vf2\n\t"
        "vaddy.x vf3, vf3, vf3y\n\t"
        "vaddz.x vf3, vf3, vf3z\n\t"
        "vaddw.x vf3, vf3, vf0w\n\t"
        "vadd.x vf3, vf3, vf3\n\t"
        "vsqrt Q, vf3x\n\t"
        "vopmula.xyz ACC, vf1, vf2\n\t"
        "vopmsub.xyz vf5, vf2, vf1\n\t"
        "vwaitq\n\t"
        "vaddq.x vf3, vf0, Q\n\t"
        "qmfc2.ni $8, vf3\n\t"
        "mtc1 $8, %0"
        : "=f"(arc.x) : "r"(from), "r"(to) : "$8", "memory");
    arc.x = __builtin_fabsf(arc.x);
    if (arc.x < 0.01f) {
        __asm__ __volatile__(
            "vsub.xyzw vf4, vf0, vf0\n\t"
            "vaddw.y vf4, vf0, vf0w\n\t"
            "sqc2 vf4, 0(%0)"
            : : "r"(destination) : "memory");
        return 0;
    }
    __asm__ __volatile__(
        "viaddi vi1, vi0, 8\n\t"
        "vmfir.w vf6, vi1\n\t"
        "vdiv Q, vf0w, vf3x\n\t"
        "vitof4.w vf6, vf6\n\t"
        "vmulx.w vf5, vf6, vf3x\n\t"
        "vwaitq\n\t"
        "vmulq.xyz vf5, vf5, Q\n\t"
        "sqc2 vf5, 0(%0)"
        : : "r"(destination) : "memory");
    return 1;
}

/*
 * VU0 block: loads the quaternion at source, forms its lane products and sums
 * in vf3..vf12, and stores four quadword rows of destination: a 4x4 rotation
 * matrix (row 0 holds 1 - 2(yy+zz)); the last row is vf0 = (0, 0, 0, 1). The
 * only caller, sdvDrawAlter, passes a row block and ignores the returned
 * pointer.
 */
Vector4 *MMathQuatToMatrix(Vector4 *destination, const Vector4 *source)
{
    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "vadd.xyzw vf2, vf1, vf1\n\t"
        "vaddw.xyzw vf9, vf0, vf0w\n\t"
        "vsub.xyzw vf8, vf0, vf0\n\t"
        "vmulz.xyzw vf5, vf2, vf1z\n\t"
        "vmuly.xyzw vf4, vf2, vf1y\n\t"
        "vmulx.xyzw vf3, vf2, vf1x\n\t"
        "vsub.xyzw vf7, vf0, vf0\n\t"
        "vsub.xyzw vf6, vf0, vf0\n\t"
        "vaddz.y vf10, vf4, vf5z\n\t"
        "vaddx.z vf11, vf5, vf3x\n\t"
        "vaddy.x vf12, vf3, vf4y\n\t"
        "vaddw.x vf8, vf5, vf4w\n\t"
        "vsubw.x vf7, vf4, vf5w\n\t"
        "vsuby.x vf6, vf9, vf10y\n\t"
        "vsubw.y vf8, vf5, vf3w\n\t"
        "vsubz.y vf7, vf9, vf11z\n\t"
        "vaddw.y vf6, vf3, vf5w\n\t"
        "vsubx.z vf8, vf9, vf12x\n\t"
        "vaddw.z vf7, vf4, vf3w\n\t"
        "vsubw.z vf6, vf3, vf4w\n\t"
        "sqc2 vf0, 48(%0)\n\t"
        "sqc2 vf8, 32(%0)\n\t"
        "sqc2 vf7, 16(%0)\n\t"
        "sqc2 vf6, 0(%0)"
        : : "r"(destination), "r"(source) : "memory");
    return destination;
}

/*
 * The same VU0 operations as MMathQuatToMatrix, with the three row zeroings and
 * the vf9 sum scheduled differently. No caller in this unit.
 */
Vector4 *func_00272EC8(Vector4 *destination, const Vector4 *source)
{
    __asm__ __volatile__(
        "lqc2 vf1, 0(%1)\n\t"
        "vadd.xyzw vf2, vf1, vf1\n\t"
        "vsub.xyzw vf8, vf0, vf0\n\t"
        "vsub.xyzw vf7, vf0, vf0\n\t"
        "vsub.xyzw vf6, vf0, vf0\n\t"
        "vmulz.xyzw vf5, vf2, vf1z\n\t"
        "vmuly.xyzw vf4, vf2, vf1y\n\t"
        "vmulx.xyzw vf3, vf2, vf1x\n\t"
        "vaddw.xyzw vf9, vf0, vf0w\n\t"
        "vaddz.y vf10, vf4, vf5z\n\t"
        "vaddx.z vf11, vf5, vf3x\n\t"
        "vaddy.x vf12, vf3, vf4y\n\t"
        "vaddw.x vf8, vf5, vf4w\n\t"
        "vsubw.x vf7, vf4, vf5w\n\t"
        "vsuby.x vf6, vf9, vf10y\n\t"
        "vsubw.y vf8, vf5, vf3w\n\t"
        "vsubz.y vf7, vf9, vf11z\n\t"
        "vaddw.y vf6, vf3, vf5w\n\t"
        "vsubx.z vf8, vf9, vf12x\n\t"
        "vaddw.z vf7, vf4, vf3w\n\t"
        "vsubw.z vf6, vf3, vf4w\n\t"
        "sqc2 vf0, 48(%0)\n\t"
        "sqc2 vf8, 32(%0)\n\t"
        "sqc2 vf7, 16(%0)\n\t"
        "sqc2 vf6, 0(%0)"
        : : "r"(destination), "r"(source) : "memory");
    return destination;
}
