#ifndef INCLUDE_MAIN_TU194_H
#define INCLUDE_MAIN_TU194_H

/* Matrix helpers of src/main/tu194.c that the effect files call. */

Vector4 *MMathApplyMatrix(Vector4 *out, Matrix4 *matrix, const Vector4 *vector);
Matrix4 *MMathScaleMatrix(Matrix4 *out, Matrix4 *matrix, const Vector4 *scale);

#endif /* INCLUDE_MAIN_TU194_H */
