#include "common.h"
#include "shared.h"

/* Current matrix = current x M, M given by four rows (vf26-vf29 are the current matrix). */
void vu0CurMatrixMulRight(Vector4 *rows) {
    __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(rows) : "memory");
    __asm__ __volatile__("lqc2 vf2, 16(%0)" : : "r"(rows) : "memory");
    __asm__ __volatile__("lqc2 vf3, 32(%0)" : : "r"(rows) : "memory");
    __asm__ __volatile__("lqc2 vf4, 48(%0)" : : "r"(rows) : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf26x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf26y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf26z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf26, vf4, vf26w" : : : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf27x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf27y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf27z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf27, vf4, vf27w" : : : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf28x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf28y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf28z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf28, vf4, vf28w" : : : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf29x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf29y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf29z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf29, vf4, vf29w" : : : "memory");
}

/* Current matrix = current x [M; (0,0,0,1)] for three argument rows; row 3 is untouched. */
void vu0CurMatrixMulRight3(Vector4 *rows) {
    __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(rows) : "memory");
    __asm__ __volatile__("lqc2 vf2, 16(%0)" : : "r"(rows) : "memory");
    __asm__ __volatile__("lqc2 vf3, 32(%0)" : : "r"(rows) : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf26x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf26y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf26z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf26, vf0, vf26w" : : : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf27x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf27y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf27z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf27, vf0, vf27w" : : : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf1, vf28x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf2, vf28y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf3, vf28z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf28, vf0, vf28w" : : : "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/tu152", vu0SetCurrentRotationX);

INCLUDE_ASM("asm/main/nonmatchings/tu152", vu0SetCurrentRotationY);

INCLUDE_ASM("asm/main/nonmatchings/tu152", vu0SetCurrentRotationZ);
