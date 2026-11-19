#ifndef INCLUDE_MAIN_TU192_H
#define INCLUDE_MAIN_TU192_H

/* Functions of src/main/tu192.c that other files call. */

float MMathVectorDotProduct(const Vector4 *first, const Vector4 *second);
Vector4 *MMathVectorCrossProduct(Vector4 *destination, const Vector4 *first, const Vector4 *second);
Vector4 *MMathVectorInterpolation(Vector4 *destination, const Vector4 *first, const Vector4 *second, float parameter);

#endif /* INCLUDE_MAIN_TU192_H */
