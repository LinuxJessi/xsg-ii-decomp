#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu196", MMathRotTransPersClip);

INCLUDE_ASM("asm/main/nonmatchings/tu196", MMathQuatBetweenVectors);

#include "shared.h"

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
