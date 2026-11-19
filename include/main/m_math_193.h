#ifndef INCLUDE_MAIN_M_MATH_193_H
#define INCLUDE_MAIN_M_MATH_193_H

#include "shared.h"

/* The rotation helpers of src/main/m_math_193.c: each rotates matrix (the unit matrix when matrix is null)
 * about the axes in its name, applied in name order, and stores the result in out, which it returns. The
 * single-axis forms take one angle; the others the x, y and z angles of angles.
 * The source matrix is not declared const: in C a `Matrix4 *` argument does not convert to a pointer to a
 * const array without a diagnostic, and most callers pass a matrix they also write. */
Matrix4 *MMathRotateMatrixX(Matrix4 *out, Matrix4 *matrix, float angle);
Matrix4 *MMathRotateMatrixY(Matrix4 *out, Matrix4 *matrix, float angle);
Matrix4 *MMathRotateMatrixZ(Matrix4 *out, Matrix4 *matrix, float angle);
Matrix4 *MMathRotateMatrixXY(Matrix4 *out, Matrix4 *matrix, const Vector4 *angles);
Matrix4 *MMathRotateMatrixYX(Matrix4 *out, Matrix4 *matrix, const Vector4 *angles);
Matrix4 *MMathRotateMatrixXYZ(Matrix4 *out, Matrix4 *matrix, const Vector4 *angles);
Matrix4 *MMathRotateMatrixYXZ(Matrix4 *out, Matrix4 *matrix, const Vector4 *angles);
Matrix4 *MMathRotateMatrixZXY(Matrix4 *out, Matrix4 *matrix, const Vector4 *angles);
Matrix4 *MMathRotateMatrixZYX(Matrix4 *out, Matrix4 *matrix, const Vector4 *angles);

#endif /* INCLUDE_MAIN_M_MATH_193_H */
