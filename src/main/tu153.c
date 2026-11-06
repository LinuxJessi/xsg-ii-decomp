#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu153", vu0RotateCurrentX);

INCLUDE_ASM("asm/main/nonmatchings/tu153", vu0RotateCurrentY);

INCLUDE_ASM("asm/main/nonmatchings/tu153", vu0RotateCurrentZ);

#include "shared.h"

void vu0RotateCurrentX(f32 value);
void vu0RotateCurrentY(f32 value);
void vu0RotateCurrentZ(f32 value);

/* Euler rotation of the current matrix: X, then Y, then Z. */
void vu0RotateCurrentXYZ(Vector4 *angles)
{
    vu0RotateCurrentX(angles->x);
    vu0RotateCurrentY(angles->y);
    vu0RotateCurrentZ(angles->z);
}

/* Same three rotations in the order Y, X, Z. */
void vu0RotateCurrentYXZ(Vector4 *angles)
{
    vu0RotateCurrentY(angles->y);
    vu0RotateCurrentX(angles->x);
    vu0RotateCurrentZ(angles->z);
}

/* Same three rotations in the order Z, Y, X. */
void vu0RotateCurrentZYX(Vector4 *angles)
{
    vu0RotateCurrentZ(angles->z);
    vu0RotateCurrentY(angles->y);
    vu0RotateCurrentX(angles->x);
}

void vu0SetCurrentRotationY(f32 value);

/* Sets the current matrix to a Y rotation, then rotates it by X and Z. */
void vu0SetCurrentRotationYXZ(Vector4 *angles)
{
    vu0SetCurrentRotationY(angles->y);
    vu0RotateCurrentX(angles->x);
    vu0RotateCurrentZ(angles->z);
}

/* Transforms a point by the current matrix; the result stays in vf29. */
void vu0TransformPoint(Vector4 *point)
{
    __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(point) : "memory");
    __asm__ __volatile__("vmulax.xyzw ACC, vf26, vf1x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf27, vf1y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf28, vf1z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf29, vf29, vf1w" : : : "memory");
}

extern Vector4 vu0IdentityMatrix[3];

/* Multiplies the current matrix by a translation: identity rows 0-2 and offset as row 3. */
void vu0TranslateCurrent(Vector4 *offset)
{
    __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(vu0IdentityMatrix) : "memory");
    __asm__ __volatile__("lqc2 vf2, 16(%0)" : : "r"(vu0IdentityMatrix) : "memory");
    __asm__ __volatile__("lqc2 vf3, 32(%0)" : : "r"(vu0IdentityMatrix) : "memory");
    __asm__ __volatile__("lqc2 vf4, 0(%0)" : : "r"(offset) : "memory");
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

void func_00252518(void) {

}

INCLUDE_ASM("asm/main/nonmatchings/tu153", vu0SetRotationFromDirection);

void MMathCalcDist(void *aux, int value);

/* Copies a matrix: rows 0-2 from src, row 3 from the translation vector in aux. */
void vu0CopyMatrixWithTranslation(Vector4 *dst, Vector4 *src, Vector4 *aux, int value)
{
    Vector4 *source;
    Vector4 *translation = aux;
    Vector4 *destination = dst;
    MMathCalcDist(translation, value);
    source = src;
    __asm__ __volatile__("lqc2 vf26, 0(%0)" : : "r"(source) : "memory");
    __asm__ __volatile__("lqc2 vf27, 16(%0)" : : "r"(source) : "memory");
    __asm__ __volatile__("lqc2 vf28, 32(%0)" : : "r"(source) : "memory");
    __asm__ __volatile__("lqc2 vf29, 0(%0)" : : "r"(translation) : "memory");
    __asm__ __volatile__("sqc2 vf26, 0(%0)" : : "r"(destination) : "memory");
    __asm__ __volatile__("sqc2 vf27, 16(%0)" : : "r"(destination) : "memory");
    __asm__ __volatile__("sqc2 vf28, 32(%0)" : : "r"(destination) : "memory");
    __asm__ __volatile__("sqc2 vf29, 48(%0)" : : "r"(destination) : "memory");
}
