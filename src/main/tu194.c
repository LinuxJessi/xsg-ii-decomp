#include "common.h"
#include "shared.h"

Matrix4 *MMathScaleMatrix(Matrix4 *out, Matrix4 *matrix, const Vector4 *scale);

Vector4 *MMathApplyMatrix(Vector4 *out, Matrix4 *matrix, const Vector4 *vector)
{
    __asm__ __volatile__(
        "lqc2 $vf1,0(%2)\n\t"
        "lqc2 $vf2,0(%1)\n\t"
        "lqc2 $vf3,16(%1)\n\t"
        "lqc2 $vf4,32(%1)\n\t"
        "lqc2 $vf5,48(%1)\n\t"
        "vmulax.xyzw ACCxyzw,$vf2xyzw,$vf1x\n\t"
        "vmadday.xyzw ACCxyzw,$vf3xyzw,$vf1y\n\t"
        "vmaddaz.xyzw ACCxyzw,$vf4xyzw,$vf1z\n\t"
        "vmaddw.xyzw $vf1xyzw,$vf5xyzw,$vf1w\n\t"
        "sqc2 $vf1,0(%0)"
        : : "r"(out), "r"(matrix), "r"(vector) : "memory");
    return out;
}

/*
 * MMathMulMatrix (X1 computes the same 4x4 product): out receives the product lhs x rhs in the
 * row-vector convention of MMathApplyMatrix (each row of lhs times rhs). Returns out.
 */
Matrix4 *MMathMulMatrix(Matrix4 *out, const Matrix4 *rhs, const Matrix4 *lhs) {
    __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(rhs) : "memory");
    __asm__ __volatile__("lqc2 vf2, 16(%0)" : : "r"(rhs) : "memory");
    __asm__ __volatile__("lqc2 vf3, 32(%0)" : : "r"(rhs) : "memory");
    __asm__ __volatile__("lqc2 vf4, 48(%0)" : : "r"(rhs) : "memory");
    __asm__ __volatile__("lqc2 vf5, 0(%0)" : : "r"(lhs) : "memory");
    __asm__ __volatile__("lqc2 vf6, 16(%0)" : : "r"(lhs) : "memory");
    __asm__ __volatile__("lqc2 vf7, 32(%0)" : : "r"(lhs) : "memory");
    __asm__ __volatile__("lqc2 vf8, 48(%0)" : : "r"(lhs) : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf5x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf5y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf5z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf5, vf4, vf5w" : : : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf6x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf6y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf6z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf6, vf4, vf6w" : : : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf7x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf7y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf7z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf7, vf4, vf7w" : : : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf8x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf8y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf8z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf8, vf4, vf8w" : : : "memory");
    __asm__ __volatile__("sqc2 vf5, 0(%0)" : : "r"(out) : "memory");
    __asm__ __volatile__("sqc2 vf6, 16(%0)" : : "r"(out) : "memory");
    __asm__ __volatile__("sqc2 vf7, 32(%0)" : : "r"(out) : "memory");
    __asm__ __volatile__("sqc2 vf8, 48(%0)" : : "r"(out) : "memory");
    return out;
}

Matrix4 *MMathScaleMatrix(Matrix4 *out, Matrix4 *matrix, const Vector4 *scale)
{
    __asm__ __volatile__(
        "lqc2 $vf5,0(%2)\n\t"
        "lqc2 $vf1,0(%1)\n\t"
        "lqc2 $vf2,16(%1)\n\t"
        "lqc2 $vf3,32(%1)\n\t"
        "lqc2 $vf4,48(%1)\n\t"
        "vmulx.xyzw $vf1xyzw,$vf1xyzw,$vf5x\n\t"
        "vmuly.xyzw $vf2xyzw,$vf2xyzw,$vf5y\n\t"
        "vmulz.xyzw $vf3xyzw,$vf3xyzw,$vf5z\n\t"
        "vmulw.xyzw $vf4xyzw,$vf4xyzw,$vf5w\n\t"
        "sqc2 $vf1,0(%0)\n\t"
        "sqc2 $vf2,16(%0)\n\t"
        "sqc2 $vf3,32(%0)\n\t"
        "sqc2 $vf4,48(%0)"
        : : "r"(out), "r"(matrix), "r"(scale) : "memory");
    return out;
}
