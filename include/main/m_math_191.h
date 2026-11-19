#ifndef INCLUDE_MAIN_M_MATH_191_H
#define INCLUDE_MAIN_M_MATH_191_H

/* The vector helpers of src/main/m_math_191.c that other files call. */

/* MMathCalcAngle: pitch to out->x, yaw to out->y; returns out. */
Vector4 *MMathCalcAngle(Vector4 *out, const Vector4 *source, const Vector4 *target);

Vector4 *MMathSubVectorMulS(Vector4 *destination, const Vector4 *first, const Vector4 *second, float scale);

#endif /* INCLUDE_MAIN_M_MATH_191_H */
