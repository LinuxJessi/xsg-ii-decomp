#ifndef INCLUDE_MAIN_TU196_H
#define INCLUDE_MAIN_TU196_H

#include "shared.h"

struct XglCamera;

/* Functions of src/main/tu196.c that other files call. */

/* Transforms point by matrix, projects it with the camera's screen scale and offset and stores the fixed-point
 * screen position in destination, with the clip mark in w (0x8000 when clipped, else 0), which it returns.
 * The matrix is not declared const for the reason given in main/m_math_193.h. */
int MMathRotTransPersClip(Vector4 *destination, const struct XglCamera *camera, Matrix4 *matrix, const Vector4 *point);
/* Shortest-arc rotation quaternion from the unit vector from to the unit vector to; 0 when they are nearly opposite. */
int MMathQuatBetweenVectors(Vector4 *destination, const Vector4 *from, const Vector4 *to);
/* The rotation matrix rows of the quaternion source, stored at destination. */
Vector4 *MMathQuatToMatrix(Vector4 *destination, const Vector4 *source);

#endif /* INCLUDE_MAIN_TU196_H */
