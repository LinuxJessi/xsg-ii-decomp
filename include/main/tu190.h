#ifndef INCLUDE_MAIN_TU190_H
#define INCLUDE_MAIN_TU190_H

#include "shared.h"

/* The distance helpers of src/main/tu190.c that other files call. */

/* Distance from first to second, and the same in the XZ plane. */
float MMathCalcDist(const Vector4 *first, const Vector4 *second);
float MMathCalcDistXZ(const Vector4 *first, const Vector4 *second);
/* Unit vector from source towards target, stored at destination. */
Vector4 *MMathCalcDirVector(Vector4 *destination, const Vector4 *source, const Vector4 *target);

#endif /* INCLUDE_MAIN_TU190_H */
