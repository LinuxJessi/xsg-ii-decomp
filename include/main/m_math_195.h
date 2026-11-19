#ifndef INCLUDE_MAIN_M_MATH_195_H
#define INCLUDE_MAIN_M_MATH_195_H

/* The matrix helpers of src/main/m_math_195.c that other files call. */

Matrix4 *MMathScaleMatrixRotS(Matrix4 *out, Matrix4 *matrix, float scale);
Matrix4 *MMathTranslateMatrix(Matrix4 *out, Matrix4 *matrix, const Vector4 *translation);

#endif /* INCLUDE_MAIN_M_MATH_195_H */
