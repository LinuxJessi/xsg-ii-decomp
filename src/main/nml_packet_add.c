#include "common.h"
#include "shared.h"

#include "vu0_macro.h"

void _CurApplyMatrix(Vector4 *destination, const Vector4 *vector) {
    Vu0ApplyCurrentMatrix(destination, vector);
}

/* _CurApplyMatrix33: _CurApplyMatrix33 (medium); applies the current matrix with xyzw lanes and no vf30 term. */
INCLUDE_ASM("asm/main/nonmatchings/nml_packet_add", _CurApplyMatrix33);

void _CurMatrixMul(Matrix4 matrix)
{
    __asm__ __volatile__(
        "lqc2 vf2, 0(%0)\n\t"
        "lqc2 vf3, 16(%0)\n\t"
        "lqc2 vf4, 32(%0)\n\t"
        "lqc2 vf5, 48(%0)\n\t"
        "vmulax.xyzw ACC, vf27, vf2x\n\t"
        "vmadday.xyzw ACC, vf28, vf2y\n\t"
        "vmaddaz.xyzw ACC, vf29, vf2z\n\t"
        "vmaddw.xyzw vf2, vf30, vf2w\n\t"
        "vmulax.xyzw ACC, vf27, vf3x\n\t"
        "vmadday.xyzw ACC, vf28, vf3y\n\t"
        "vmaddaz.xyzw ACC, vf29, vf3z\n\t"
        "vmaddw.xyzw vf3, vf30, vf3w\n\t"
        "vmulax.xyzw ACC, vf27, vf4x\n\t"
        "vmadday.xyzw ACC, vf28, vf4y\n\t"
        "vmaddaz.xyzw ACC, vf29, vf4z\n\t"
        "vmaddw.xyzw vf4, vf30, vf4w\n\t"
        "vmulax.xyzw ACC, vf27, vf5x\n\t"
        "vmadday.xyzw ACC, vf28, vf5y\n\t"
        "vmaddaz.xyzw ACC, vf29, vf5z\n\t"
        "vmaddw.xyzw vf30, vf30, vf5w\n\t"
        "vmulw.xyzw vf27, vf2, vf0w\n\t"
        "vmulw.xyzw vf28, vf3, vf0w\n\t"
        "vmulw.xyzw vf29, vf4, vf0w"
        :
        : "r"(matrix)
        : "memory"
    );
}
