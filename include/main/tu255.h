#ifndef INCLUDE_MAIN_TU255_H
#define INCLUDE_MAIN_TU255_H

#include "shared.h"

/* Functions of src/main/tu255.c that other files call. */

void xglCurMatrixGet(f32 matrix[4][4]);
void xglCurApplyVector(Vector4 *destination, const Vector4 *vector);
void xglCurSetRotateX(float value);
void xglCurSetRotateY(float angle);
void xglCurMatrixMul(Matrix4 matrix);

#endif /* INCLUDE_MAIN_TU255_H */
